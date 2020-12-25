#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif
	void IPC_INIT();

	int send_command(uint8_t* buffer, uint32_t size);

	int recv_result(uint8_t* dst, uint32_t size);
#ifdef __cplusplus
}
#endif