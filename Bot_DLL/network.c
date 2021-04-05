#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h> //DEBUG
#include "network.h"

const size_t BOTNET_PACK_SIZE = sizeof(struct botnet_pack);
struct network_adapter DEC_ADAPTER_IPS_ARR[20];
size_t adapters_count = 0;

int check(int exp, const char* msg)
{
	if (exp == SOCKET_ERROR)
	{
		perror(msg);
		exit(1);
	}
	return exp;
}

void init_winsock()
{
	WSADATA data;
	WORD ver = MAKEWORD(2, 2);
	int wsock = WSAStartup(ver, &data);

	check(wsock, "init_winsock error");
}

void init_network_settings()
{
	init_winsock();

	SOCKET sd = WSASocket(AF_INET, SOCK_DGRAM, 0, 0, 0, 0);
	check(sd, "error init_network_settings building socket");

	INTERFACE_INFO InterfaceList[20];
	int nBytesReturned;

	check(WSAIoctl(sd, SIO_GET_INTERFACE_LIST, 0, 0, &InterfaceList,
		sizeof(InterfaceList), &nBytesReturned, 0, 0), "init_network_settings WSALoctl error ");
	
	//First one is local host.
	for (size_t i = 1; i < nBytesReturned / sizeof(INTERFACE_INFO); i++)
	{
		if (InterfaceList[i].iiFlags & IFF_UP)
		{
			struct network_adapter adapter;
			adapter.hip = InterfaceList[i].iiAddress.AddressIn.sin_addr.S_un.S_addr;
			adapter.netmask = InterfaceList[i].iiNetmask.AddressIn.sin_addr.S_un.S_addr;
			adapter.broadcast = InterfaceList[i].iiBroadcastAddress.AddressIn.sin_addr.S_un.S_addr;

			DEC_ADAPTER_IPS_ARR[adapters_count] = adapter;
			adapters_count++;
		}
	}	

	closesocket(sd);
}