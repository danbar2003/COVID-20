#include "ipc_manage.h"
#include <stdio.h>

static HANDLE hCreateNamedPipe;

BOOL init()
{
	// CreateFile for Pipe
	hCreateNamedPipe =  CreateNamedPipe(
			L"\\\\.\\pipe\\MYNAMEDPIPE",
			PIPE_ACCESS_DUPLEX,
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
			PIPE_UNLIMITED_INSTANCES,
			1024,
			1024,
			0,
			NULL
		);

	if (hCreateNamedPipe == INVALID_HANDLE_VALUE) {
#ifdef _DEBUG
		printf("%s %d\n", "NamedPipeCreateFile.txt file creation has failed with error number:", GetLastError());
#endif
		return 1;
	}

	ConnectNamedPipe(hCreateNamedPipe, NULL);
	return 0;
}

DWORD WINAPI send_command(u_char* buffer, size_t size)
{
	BOOL bWriteFile;
	DWORD dwNoBytesWrite;

	bWriteFile = WriteFile(
		hCreateNamedPipe,
		buffer,
		size,
		&dwNoBytesWrite,
		NULL
	);

	if (bWriteFile == FALSE) {
#ifdef _DEBUG
		printf("%s %d\n", "ReadFile has failed with error number:", GetLastError());
#endif
		return 1;
	}
	return 0;
}

DWORD WINAPI get_result(u_char* buffer, size_t size)
{
	BOOL bReadFile;
	DWORD dwNoBytesRead;
	// ReadFile
	bReadFile = ReadFile(
		hCreateNamedPipe,
		buffer,
		size,
		&dwNoBytesRead,
		NULL
	);

	if (bReadFile == FALSE) {

#ifdef _DEBUG
		printf("%s %d\n", "ReadFile has failed with error number:", GetLastError());
#endif
		return 1;
	}
	return 0;
}