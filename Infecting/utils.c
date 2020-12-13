#include "utils.h"

PIP_ADAPTER_INFO correspoding_adapter(pcap_if_t* pcap_adapter, PIP_ADAPTER_INFO pAdapters)
{
	const char* str1 = pcap_adapter->name;
	const char* str2;
	size_t i;

	str1 += strlen(str1) - 1;
	while (pAdapters)
	{
		i = 0;
		str2 = pAdapters->AdapterName;
		str2 += strlen(str2) - 1;

		while (*(str1 - i) == *(str2 - i) && *(str1 - i) != '{')
			i++;

		if (*(str1 - i) == *(str2 - i))
			return pAdapters;

		pAdapters = pAdapters->Next;
	}
	return NULL;
}