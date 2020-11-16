#include "ipc_manage.h"
#include <stdio.h> //DEBUG

static HANDLE hCreateFile;

void connect_to_parent_process()
{
	//connect to the main process
	hCreateFile = CreateFile(
		L"\\\\.\\pipe\\COVID-20",
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	//check if connection succeeded
	if (INVALID_HANDLE_VALUE == hCreateFile) {
#ifdef _DEBUG
		printf("%s %d\n", "NamedPipeCreateFile.txt file creation has failed with error number:", GetLastError());
		exit(1);
#endif _DEBUG
	}
}

BOOL get_command(u_char* buffer, size_t size)
{
	BOOL bReadFile;
	DWORD dwNoBytesRead;
	// ReadFile
	bReadFile = ReadFile(
		hCreateFile,
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