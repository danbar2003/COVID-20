#include "network.h"
#include "communication.h"
#include "ipc_manage.h"

#include <stdio.h>
#include <Windows.h>

int main(int argc, char* argv[])
{
	init_network_settings();
	create_communication_sockets();
	//IPC_INIT();
	HANDLE communication = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)handle_incomings, NULL, 0, NULL);
	peer_request();
	
	
	getc(stdin);
}