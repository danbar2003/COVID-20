#pragma once

#include <Windows.h>

#define ACTS_NUM 2 //scan, infect

enum ACTS
{
	SCAN, //start scanning the net
	INFECT, //start infecting
	STOP_I, //stop infecting
	STOP_S, //stop scanning
	STOP_A //stop all
};

typedef struct command
{
	BYTE act;
	ULONG32 ip_addr; 
	ULONG32 netmask;
	//etc...
} *pCommand;


void execute_command(pCommand command);