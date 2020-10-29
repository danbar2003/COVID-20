#include "Communication.h"
#include "network.h"

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
	
	//tcp_sock = tcp;
	NetTree = new NetworkTree(udp_sock, tcp_sock);


}

Communication::~Communication()
{
	delete NetTree;
}