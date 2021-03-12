#pragma once

#define HAVE_REMOTE
#include <pcap.h>

#include <ws2tcpip.h>
#include <iphlpapi.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

#include <Windows.h>
