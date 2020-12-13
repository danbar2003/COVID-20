#pragma once

#include <pcap/pcap.h>
#include <iphlpapi.h>

PIP_ADAPTER_INFO corresponding_adapter(pcap_if_t* pcap_adapter, PIP_ADAPTER_INFO pAdapters);