#include "ov/tray.h"
#include "ov/win.h"
#include "log.h"
#include <shellapi.h>

namespace ov {

void tray_t::add(HWND h) {
    hwnd = h;
    auto nid = NOTIFYICONDATAW{};
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = MSG_TRAY;
    nid.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wcscpy_s(nid.szTip, L"overlay");
    Shell_NotifyIconW(NIM_ADD, &nid);
    LOGS("tray icon added");
}

void tray_t::del() {
    auto nid = NOTIFYICONDATAW{};
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = 1;
    Shell_NotifyIconW(NIM_DELETE, &nid);
    LOGS("tray icon deleted");
}

void tray_t::show(const wchar_t *title, const wchar_t *text) {
    auto nid = NOTIFYICONDATAW{};
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_INFO;
    nid.dwInfoFlags = NIIF_INFO;
    nid.uTimeout = 3000;
    wcscpy_s(nid.szInfoTitle, title);
    wcscpy_s(nid.szInfo, text);
    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

}