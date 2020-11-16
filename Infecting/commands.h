#pragma once

#include <Windows.h>

enum ACTS
{
	SCAN = 1, //start scanning the net
	INFECT, //start infecting
	STOP_I, //stop infecting
	STOP_S, //stop scanning
	STOP_A //stop all
};

typedef struct command
{
	BYTE act;
	ULONG32 victim; //for infecting
	//etc...
} *pCommand;


void execute_command(pCommand command);