#pragma once

#include <Windows.h>

#ifdef __cplusplus
extern "C"
{
#endif
BOOL IPC_init();

BOOL send_command(u_char* buffer, size_t size);

DWORD get_result(u_char* buffer, size_t size);
#ifdef __cplusplus
}
#endif