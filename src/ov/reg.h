#pragma once
#include <windows.h>
#include <string>

namespace ov {

std::wstring reg_read(const wchar_t *key, const wchar_t *def);
DWORD reg_read_dw(const wchar_t *key, DWORD def);
void reg_write(const wchar_t *key, const std::wstring &val);
void reg_write_dw(const wchar_t *key, DWORD val);

}
