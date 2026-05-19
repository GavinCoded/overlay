#include "ov/proc.h"
#include "ov/win.h"
#include "ov/view.h"
#include "ov/tray.h"
#include "log.h"
#include <shellapi.h>

namespace ov {

extern win_t *g_win;

LRESULT CALLBACK wnd_proc(HWND h, UINT m, WPARAM w, LPARAM l) {
    auto ww = (win_t *)GetWindowLongPtr(h, GWLP_USERDATA);
    switch (m) {
    case WM_HOTKEY: if (ww && !ww->paused) ww->tog_ch(); return 0;
    case WM_CHAR:
        if (ww && ww->open && !ww->cd)
            { ww->chat.write((wchar_t)w); render(*ww); }
        return 0;
    case MSG_ENTER:
        if (ww && ww->open && !ww->cd) {
            auto t = ww->chat.input;
            if (!t.empty() && ww->cli) {
                auto len = WideCharToMultiByte(CP_UTF8, 0, t.c_str(), -1, nullptr, 0, nullptr, nullptr);
                std::string n(len, 0);
                WideCharToMultiByte(CP_UTF8, 0, t.c_str(), -1, n.data(), len, nullptr, nullptr);
                n.resize(len - 1); ww->cli->send(n);
            }
            if (ww->hook) { UnhookWindowsHookEx(ww->hook); ww->hook = nullptr; }
            KillTimer(h, TIMER_CURSOR); ww->chat.cursor_on = false; ww->cd = true;
            if (!ww->paused) RegisterHotKey(h, 1, MOD_NOREPEAT, ww->kvk);
            SetTimer(h, TIMER_CLOSE, 5000, nullptr);
            ww->chat.input.clear(); ww->chat.cursor = 0;
            render(*ww);
        }
        return 0;
    case MSG_ESC: if (ww) ww->close_ch(); return 0;
    case MSG_BACK:
        if (ww && ww->open && !ww->cd) { ww->chat.back(); render(*ww); }
        return 0;
    case MSG_TRAY:
        if (l == WM_RBUTTONUP && ww) {
            POINT pt; GetCursorPos(&pt);
            auto menu = CreatePopupMenu();
            AppendMenuW(menu, MF_STRING, MENU_RECONNECT, L"reconnect");
            AppendMenuW(menu, ww->paused ? MF_CHECKED : MF_STRING, MENU_PAUSE,
                ww->paused ? L"keybind paused" : L"pause keybind");
            AppendMenuW(menu, MF_STRING, MENU_EXIT, L"exit");
            SetForegroundWindow(h);
            auto cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY,
                pt.x, pt.y, 0, h, nullptr);
            DestroyMenu(menu);
            if (cmd == MENU_RECONNECT) PostMessage(h, MSG_RECONNECT, 1, 0);
            else if (cmd == MENU_EXIT) PostMessage(h, MSG_RECONNECT, 0, 0);
            else if (cmd == MENU_PAUSE) {
                ww->paused = !ww->paused;
                if (ww->paused) {
                    UnregisterHotKey(h, 1);
                    auto nid = NOTIFYICONDATAW{};
                    nid.cbSize = sizeof(nid); nid.hWnd = h; nid.uID = 1;
                    nid.uFlags = NIF_TIP;
                    wcscpy_s(nid.szTip, L"overlay (paused)");
                    Shell_NotifyIconW(NIM_MODIFY, &nid);
                } else {
                    if (!ww->open) RegisterHotKey(h, 1, MOD_NOREPEAT, ww->kvk);
                    auto nid = NOTIFYICONDATAW{};
                    nid.cbSize = sizeof(nid); nid.hWnd = h; nid.uID = 1;
                    nid.uFlags = NIF_TIP;
                    wcscpy_s(nid.szTip, L"overlay");
                    Shell_NotifyIconW(NIM_MODIFY, &nid);
                }
            }
        }
        return 0;
    case MSG_RECONNECT: PostQuitMessage((int)w); return 0;
    case MSG_AUTO_OPEN: if (ww && !ww->open) { ww->open = true; ww->cd = true; ww->chat.cursor_on = false; ShowWindow(ww->hw, SW_SHOW); SetTimer(ww->hw, TIMER_CLOSE, 5000, nullptr); render(*ww); } return 0;
    case MSG_RENDER:
        if (ww) {
            if (ww->ntf) {
                ww->ntf = false;
                auto info = std::wstring(L"connected! press ") + ww->kn + L" to open the overlay";
                auto nid = NOTIFYICONDATAW{};
                nid.cbSize = sizeof(nid); nid.hWnd = h; nid.uID = 1;
                nid.uFlags = NIF_INFO; nid.dwInfoFlags = NIIF_INFO; nid.uTimeout = 3000;
                wcscpy_s(nid.szInfoTitle, ww->srv.c_str());
                wcscpy_s(nid.szInfo, info.c_str());
                Shell_NotifyIconW(NIM_MODIFY, &nid);
            }
            if (ww->open) render(*ww);
        }
        return 0;
    case WM_TIMER:
        if (w == TIMER_CURSOR && ww && ww->open && !ww->cd)
            { ww->chat.cursor_on = !ww->chat.cursor_on; render(*ww); }
        if (w == TIMER_CLOSE && ww && ww->cd) ww->close_ch();
        return 0;
    case WM_DISPLAYCHANGE:
        if (ww) { ww->sw = GetSystemMetrics(SM_CXSCREEN); ww->sh = GetSystemMetrics(SM_CYSCREEN); }
        return 0;
    case WM_DESTROY: return 0;
    }
    return DefWindowProc(h, m, w, l);
}

}