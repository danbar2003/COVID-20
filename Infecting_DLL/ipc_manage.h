#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

	int connect_to_parent_process();

	int get_command(uint8_t* buffer, uint32_t size);

	int send_result(uint8_t* buffer, uint32_t size);
#ifdef __cplusplus
}
#endif
