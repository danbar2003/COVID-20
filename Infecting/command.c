#include <Windows.h>

#include "commands.h"

static HANDLE act_threads[ACTS_NUM];

static DWORD WINAPI scan(LPVOID lparam)
{
	pCommand command = (pCommand)lparam;
	return 0;
}

static DWORD WINAPI infect(LPVOID lparam)
{
	pCommand command = (pCommand)lparam;
	return 0;
}

void execute_command(pCommand command)
{
	
	switch (command->act)
	{
	case SCAN:
		//check if thread not active
		if (GetExitCodeThread(act_threads[SCAN], NULL) != STILL_ACTIVE)
		{
			act_threads[SCAN] = CreateThread(
				NULL,
				0,
				scan,
				(LPVOID)command,
				0,
				NULL
			);
		}
		break;
	case INFECT:
		//check if thread not active
		if (GetExitCodeThread(act_threads[INFECT], NULL) != STILL_ACTIVE)
		{
			act_threads[INFECT] = CreateThread(
				NULL,
				0,
				infect,
				(LPVOID)command,
				0,
				NULL
			);
		}
		break;
	case STOP_S:
		TerminateThread(act_threads[SCAN], 0);
		break;
	case STOP_I:
		TerminateThread(act_threads[INFECT], 0);
		break;
	case STOP_A:
		TerminateThread(act_threads[SCAN], 0);
		TerminateThread(act_threads[INFECT], 0);
		break;
	}
	
}