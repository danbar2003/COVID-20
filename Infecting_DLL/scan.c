#include "scan.h"
#include "network_headers.h"
#include "ipc_manage.h"
#include "utils.h"

static send_params send_packets_params;

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

		if (netmask == 0)
		{
			addr_lst = addr_lst->Next;
			continue;
		}

		netmask = htonl(netmask);
		target = htonl(target);
		target &= netmask;

		inet_pton(AF_INET, addr_lst->IpAddress.String, &arp_header->spa);
		while (netmask != 0xffffffff)
		{
			arp_header->tpa = htonl(target);
			pcap_sendpacket(send_packets_params.fp, packet, sizeof(packet));
			Sleep(30);
			target++;
			netmask++;
		}

		addr_lst = addr_lst->Next;
	}
	return 0;
}

int scan()
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

	for (temp = alldevs; temp; temp = temp->next)
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
			continue;
		}

		/*Set the filter*/
		if (pcap_setfilter(fp, &fcode) < 0)
		{
			fprintf(stderr, "\nError setting the filter.\n");
			/* Free the device list */
			continue;
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
					break;
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