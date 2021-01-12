#pragma once
#include <pcap/pcap.h>
#include <iphlpapi.h>

#include <Windows.h>
#include <malloc.h>

#include "network_headers.h"
#include "utils.h"

DWORD WINAPI start_arp_spoofing(LPVOID lparam);

void dns_spoofing(u_char* packet);