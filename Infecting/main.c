#include <stdio.h>

#include "ipc_manage.h"
#include "commands.h"

int main(int argc, char** argv)
{
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
		
		//execute the command
		p = (pCommand)(buffer+sizeof(int));
		add_command(p);
	}
	
}