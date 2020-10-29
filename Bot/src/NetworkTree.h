#pragma once
#include <vector>

#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

class NetworkTree
{
private:
	u_long ip_addr; //dec value of ip
	std::vector<NetworkTree>* next_hosts;

public:
	void AddData(void* data);
	
};