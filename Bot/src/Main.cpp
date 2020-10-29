#include <iostream>
#include <thread>

#include "network.h"
#include "Communication.h"

int main()
{
	init_winsock();
	init_network_settings();

	Communication *communication = new Communication();
	std::thread work = std::thread([=] { communication->HandleIncomings(); });
	communication->SyncRequest();
	std::cin.get();
}