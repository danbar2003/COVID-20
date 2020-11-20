#pragma once

#include <Windows.h>

BOOL init();

DWORD WINAPI send_command(u_char* buffer, size_t size);

DWORD WINAPI get_result(u_char* buffer, size_t size);
