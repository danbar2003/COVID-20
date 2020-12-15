#pragma once

#include <WS2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

enum PACK_TYPE
{
	//UDP
	SYNC_REQUEST, // Used when a bot wants to tell it exists
	SYNC_REPLY, // Used to send network structure syncs
	VERSION_SYNC_REQUEST, //Ask for database info 
	COMMAND,
	COMMAND_RESULT,
	//TCP
	VERSION_SYNC //Sync database info
	,
};

struct network_adapter
{
	ULONG32 hip;
	ULONG32 netmask;
	ULONG32 broadcast;
};

struct botnet_pack
{
	enum PACK_TYPE type;
	
	/*-----commnad-----*/
	BYTE act;
	ULONG32 gateway_ip;
	ULONG32 victim_ip;
	BYTE gateway_mac[6];
	BYTE victim_mac[6];
	/*-----------------*/
};

#ifdef __cplusplus
extern "C"
{
#endif

	extern const char LOCAL_IP[];
	extern const int MAX_IPSTR_BUF_SIZE; // 4 times xxx (255 fe) + 3 dots + null
	extern const size_t BOTNET_PACK_SIZE;
	extern const size_t BACKLOG;
	extern struct network_adapter DEC_ADAPTER_IPS_ARR[20];

	int check(int exp, const char *msg);
	void init_winsock();
	void init_network_settings();
#ifdef __cplusplus
}
#endif