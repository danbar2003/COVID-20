// dllmain.cpp : Defines the entry point for the DLL application.

#define SYNC_RATE_SEC 40
#define PEER_REQUEST_SEC 30

#include "network.h"
#include "communication.h"
#include "ipc_manage.h"


#include <stdio.h>
#include <time.h>
#include <Windows.h>

BOOL APIENTRY DllMain(
    HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

extern "C" __declspec(dllexport) void bot_main()
{
    init_network_settings();
    create_communication_sockets();
    IPC_INIT();
    HANDLE ipc_results = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)handle_ipc_results, NULL, 0, NULL);
    HANDLE communication = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)handle_incomings, NULL, 0, NULL);
    
    clock_t time;
    int sync_counter = 0, peer_counter = 0;

    for (;;)
    {
        time = clock();
        if (time > sync_counter * SYNC_RATE_SEC * 1000)
        {
            sync_counter++;
            sync_request();
        }
        if (time > peer_counter * PEER_REQUEST_SEC * 1000)
        {
            peer_counter++;
            peer_request();
        }
        Sleep(1000);
    }

    getc(stdin);
}

