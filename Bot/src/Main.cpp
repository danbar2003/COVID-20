#include <iostream>
#include <thread>

#include "network.h"
#include "Communication.h"

int main()
{
	init_winsock();
	init_network_settings();

	Communication *communication = new Communication();
	std::thread udp_incomings = std::thread([=] { communication->HandleIncomingsUDP(); });
	std::thread tcp_incomings = std::thread([=] { communication->HandleIncomingsTCP(); });
	std::thread command_results = std::thread([=] { communication->HandleCommandResults(); });

	communication->SyncRequest();

	std::cin.get();
}