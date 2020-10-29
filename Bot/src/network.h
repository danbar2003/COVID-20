#pragma once

#include <WS2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

struct botnet_pack
{

	BYTE sync_request : 1;
	BYTE sync_relpy : 1;
	BYTE data[65000];
};

#ifdef __cplusplus
extern "C"
{
#endif


	extern const char LOCAL_IP[];
	extern const unsigned long DEC_LOCAL_IP;
	extern const int MAX_IPSTR_BUF_SIZE; // 4 times xxx (255 fe) + 3 dots + null
	extern const size_t BOTNET_PACK_SIZE;
	extern const INTERFACE_INFO InterfaceList[20];

	int check(int exp, const char *msg);
	int init_winsock();
	int init_network_settings();
#ifdef __cplusplus
}
#endif