#pragma once
#include <windows.h>
#include <shellapi.h>

inline void notify(HWND hw, const wchar_t *title, const wchar_t *text) {
    auto nid = NOTIFYICONDATAW{};
    nid.cbSize = sizeof(nid);
    nid.hWnd = hw;
    nid.uID = 1;
    nid.uFlags = NIF_INFO;
    nid.dwInfoFlags = NIIF_INFO;
    nid.uTimeout = 2000;
    wcscpy_s(nid.szInfoTitle, title);
    wcscpy_s(nid.szInfo, text);
    Shell_NotifyIconW(NIM_MODIFY, &nid);
}
