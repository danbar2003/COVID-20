#pragma once
#include <vector>

#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

class LocalNetwork
{
private:
	u_long ip_addr; //dec value of ip
	std::vector<u_long> next_hosts;

public:

	LocalNetwork();

	void AddHost(u_long host);
	
};