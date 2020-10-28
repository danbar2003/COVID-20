#include "Communication.h"

Communication::Communication()
{

	udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (udp_sock == INVALID_SOCKET)
	{
		exit(1);
	}
	// Create a server hint structure for the server
	sockaddr_in serverHint;
	serverHint.sin_addr.S_un.S_addr = ADDR_ANY; // Us any IP address available on the machine
	serverHint.sin_family = AF_INET; // Address format is IPv4
	serverHint.sin_port = htons(54000); // Convert from little to big endian

	// Try and bind the socket to the IP and port
	if (bind(udp_sock, (sockaddr*)&serverHint, sizeof(serverHint)) == SOCKET_ERROR)
	{
		printf("%s %d\n", "can't bind socket", WSAGetLastError());
		exit(1);
	}

	tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (tcp_sock == INVALID_SOCKET)
	{
		printf("%s %d\n", "Error creating tcp_sock", WSAGetLastError());
		exit(1);
	}

	serverHint.sin_addr.S_un.S_addr = ADDR_ANY; // Us any IP address available on the machine
	serverHint.sin_family = AF_INET; // Address format is IPv4
	serverHint.sin_port = htons(55000); // Convert from little to big endian

	if (bind(tcp_sock, (sockaddr*)&serverHint, sizeof(serverHint)) == SOCKET_ERROR)
	{
		printf("%s %d\n", "can't bind socket", WSAGetLastError());
		exit(1);
	}

	listen(tcp_sock, SOMAXCONN);
	//tcp_sock = tcp;
	NetTree = new NetworkTree(udp_sock, tcp_sock);


}

Communication::~Communication()
{
	delete NetTree;
}