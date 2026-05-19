#include "ov/reg.h"

namespace {

const wchar_t *reg_path = L"Software\\OverlayChat";

}

namespace ov {

std::wstring reg_read(const wchar_t *key, const wchar_t *def) {
    HKEY hk; wchar_t buf[512] = {}; DWORD sz = sizeof(buf);
    if (RegOpenKeyExW(HKEY_CURRENT_USER, reg_path, 0, KEY_READ, &hk) != ERROR_SUCCESS) return def;
    RegQueryValueExW(hk, key, nullptr, nullptr, (BYTE *)buf, &sz);
    RegCloseKey(hk); return buf;
}

DWORD reg_read_dw(const wchar_t *key, DWORD def) {
    HKEY hk; DWORD val = def; DWORD sz = sizeof(val);
    if (RegOpenKeyExW(HKEY_CURRENT_USER, reg_path, 0, KEY_READ, &hk) != ERROR_SUCCESS) return def;
    RegQueryValueExW(hk, key, nullptr, nullptr, (BYTE *)&val, &sz);
    RegCloseKey(hk); return val;
}

void reg_write(const wchar_t *key, const std::wstring &val) {
    HKEY hk;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, reg_path, 0, nullptr,
        REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hk, nullptr) != ERROR_SUCCESS) return;
    RegSetValueExW(hk, key, 0, REG_SZ,
        (const BYTE *)val.c_str(), (DWORD)((val.size() + 1) * sizeof(wchar_t)));
    RegCloseKey(hk);
}

void reg_write_dw(const wchar_t *key, DWORD val) {
    HKEY hk;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, reg_path, 0, nullptr,
        REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hk, nullptr) != ERROR_SUCCESS) return;
    RegSetValueExW(hk, key, 0, REG_DWORD, (const BYTE *)&val, sizeof(val));
    RegCloseKey(hk);
}

}
