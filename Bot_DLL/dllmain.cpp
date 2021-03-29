// dllmain.cpp : Defines the entry point for the DLL application.

#include "network.h"
#include "communication.h"
#include "ipc_manage.h"

#include <stdio.h>
#include <Windows.h>

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    init_network_settings();
    create_communication_sockets();
    IPC_INIT();
    HANDLE ipc_results = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)handle_ipc_results, NULL, 0, NULL);
    HANDLE communication = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)handle_incomings, NULL, 0, NULL);

    sync_request();

    getc(stdin);
    return TRUE;
}

