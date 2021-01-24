#pragma once
#include <Windows.h>

void IPC_INIT();

int send_command(u_char* buffer, UINT32 size);

int recv_result(u_char* dst, UINT32 size);
