#pragma once

#include <pcap/pcap.h>
#include <iphlpapi.h>

PIP_ADAPTER_INFO correspoding_adapter(pcap_if_t* pcap_adapter, PIP_ADAPTER_INFO pAdapters);