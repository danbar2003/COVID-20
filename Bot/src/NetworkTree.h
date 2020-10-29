#pragma once
#include <vector>

#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

class NetworkTree
{
private:
	u_long ip_addr; //dec value of ip
	std::vector<NetworkTree>* next_hosts;

private:
	void send_broadcast_msg(const SOCKET& udp_sock);

	void get_network_status(const SOCKET& tcp_sock, const SOCKET& udp_sock);

public:
	NetworkTree(const SOCKET& udp_sock, const SOCKET& tcp_sock);

	
};