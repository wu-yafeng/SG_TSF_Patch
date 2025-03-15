// BootMain.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <Windows.h>

static void CreateSuspendProcessInjection(std::string cmdline, std::string dllPath) {
	// Create a new process in suspended state
	STARTUPINFOA si{};
	PROCESS_INFORMATION pi{};
	si.cb = sizeof(si);
	char cmdlineBuffer[1024];
	strcpy_s(cmdlineBuffer, cmdline.c_str());
	if (!CreateProcessA(NULL, cmdlineBuffer, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi)) {
		printf("Failed to create process: %d\n", GetLastError());
		return;
	}
	// Allocate memory in the target process
	auto pDllPath = VirtualAllocEx(pi.hProcess, NULL, dllPath.size() + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!pDllPath) {
		printf("Failed to allocate memory in target process: %d\n", GetLastError());
		TerminateProcess(pi.hProcess, 0);
		return;
	}
	// Write the DLL path to the target process
	if (!WriteProcessMemory(pi.hProcess, pDllPath, dllPath.c_str(), dllPath.size(), NULL)) {
		printf("Failed to write DLL path to target process: %d\n", GetLastError());
		TerminateProcess(pi.hProcess, 0);
		return;
	}
	// Get the address of LoadLibraryA in kernel32.dll
	auto hKernel32 = GetModuleHandleA("kernel32.dll");

	if (!hKernel32) {
		printf("Failed to get kernel32.dll handle: %d\n", GetLastError());
		TerminateProcess(pi.hProcess, 0);
		return;
	}

	auto pLoadLibraryA = GetProcAddress(hKernel32, "LoadLibraryA");
	// Create a remote thread in the target process
	auto hRemoteThread = CreateRemoteThread(pi.hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pLoadLibraryA, pDllPath, 0, NULL);
	if (!hRemoteThread) {
		printf("Failed to create remote thread: %d\n", GetLastError());
		TerminateProcess(pi.hProcess, 0);
		return;
	}
	// Wait for the remote thread to finish
	WaitForSingleObject(hRemoteThread, INFINITE);
	// Clean up
	CloseHandle(hRemoteThread);
	VirtualFreeEx(pi.hProcess, pDllPath, 0, MEM_RELEASE);
	ResumeThread(pi.hThread);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}
static std::string GetRegValue(HKEY hKeyType, DWORD dwType, std::string lpPath, std::string lpName)
{
	HKEY hKEY;
	DWORD dataSize = MAX_PATH;
	char data[MAX_PATH]{ 0 };
	std::string strValue("");
	if (RegOpenKeyExA(hKeyType, lpPath.c_str(), NULL, KEY_READ, &hKEY) == ERROR_SUCCESS)        //如果无法打开hKEY,则中止程序的执行  
	{
		long lRet = RegQueryValueExA(hKEY, lpName.c_str(), NULL, &dwType, (LPBYTE)data, &dataSize);
		if (lRet == ERROR_SUCCESS)
		{/*
			for (int i = 0; i < (int)dataSize; i++)
			{
				strValue = strValue + data[i];
			}*/

			strValue = data;
		}
		RegCloseKey(hKEY);        // 程序结束前要关闭已经打开的 hKEY。
	}
	else
	{
		RegCreateKeyExA(hKeyType, lpPath.c_str(), 0, NULL, NULL, KEY_WRITE, NULL, &hKEY, NULL);
		RegCloseKey(hKEY);        // 程序结束前要关闭已经打开的 hKEY。
	}

	return strValue;
}

static std::string GetDllPath() {
	char dllPath[MAX_PATH]{ 0 };
	GetModuleFileNameA(NULL, dllPath, MAX_PATH);
	auto lastSlash = strrchr(dllPath, '\\');
	if (lastSlash) {
		*lastSlash = '\0';
	}
	strcat_s(dllPath, "\\TSF_Patcher.dll");
	return std::string(dllPath);
}

int main()
{
	auto value = GetRegValue(HKEY_CURRENT_USER, REG_SZ, "Software\\Tencent\\QQSG\\SYS", "pathRoot");

	if (value.empty())
	{
		printf("找不到游戏目录 \n");
		system("pause");

		return -1;
	}

	auto dllPath = GetDllPath();

	// check file exist
	if (GetFileAttributesA(dllPath.c_str()) == INVALID_FILE_ATTRIBUTES)
	{
		printf("找不到dll文件 \n");
		system("pause");
		return -1;
	}

	std::string pathRoot(value.data());

	char arguments[11] =
	{
		'-',
		3,
		'.',
		1,
		4,
		1,
		5,
		9,
		2,
		6,
		0
	};

	auto commandline = (pathRoot + "QQSG.exe " + arguments);

	CreateSuspendProcessInjection(commandline, dllPath);

	system("pause");
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
