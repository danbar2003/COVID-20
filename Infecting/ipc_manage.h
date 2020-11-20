#pragma once
#include <Windows.h>

void connect_to_parent_process();

BOOL get_command(u_char* buffer, size_t size);

BOOL send_result(u_char* buffer, size_t size);