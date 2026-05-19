#include "ov/proc.h"
#include "ov/win.h"
#include "ov/view.h"
#include "ov/notify.h"
#include "log.h"

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
    case MSG_PASTE:
        if (ww && ww->open && !ww->cd && OpenClipboard(nullptr)) {
            auto handle = GetClipboardData(CF_UNICODETEXT);
            if (handle) {
                auto text = (const wchar_t *)GlobalLock(handle);
                if (text) { for (auto p = text; *p; p++) ww->chat.write(*p); render(*ww); GlobalUnlock(handle); }
            }
            CloseClipboard();
        }
        return 0;
    case MSG_RECONNECT_ATTEMPT:
        if (ww && ww->cli && ww->cli->should_reconnect()) {
            LOGS("MSG_RECONNECT_ATTEMPT: should reconnect, setting 3s timer");
            notify(h, L"Overlay", L"Disconnected. Reconnecting in 3 seconds...");
            SetTimer(h, TIMER_RECONNECT, 3000, nullptr);
        } else {
            LOG("MSG_RECONNECT_ATTEMPT: should_reconnect=%d, not attempting", ww ? (ww->cli ? ww->cli->should_reconnect() : -1) : -2);
        }
        return 0;
    case MSG_RENDER:
        if (ww) {
            if (ww->ntf) {
                ww->ntf = false;
                LOGS("MSG_RENDER: showing connected notification");
                auto info = std::wstring(L"connected! press ") + ww->kn + L" to open the overlay";
                notify(h, ww->srv.c_str(), info.c_str());
            }
            if (ww->open) render(*ww);
        }
        return 0;
    case WM_TIMER:
        if (w == TIMER_CURSOR && ww && ww->open && !ww->cd)
            { ww->chat.cursor_on = !ww->chat.cursor_on; render(*ww); }
        if (w == TIMER_CLOSE && ww && ww->cd) ww->close_ch();
        if (w == TIMER_RECONNECT && ww && ww->cli && ww->cli->should_reconnect()) {
            LOGS("timer: TIMER_RECONNECT fired, attempting reconnect");
            KillTimer(h, TIMER_RECONNECT);
            ww->cli->stop();
            LOG("timer: reconnecting to %s:%d as '%s'", ww->cli->last_host().c_str(), ww->cli->last_port(), ww->cli->last_user().c_str());
            if (ww->cli->go(ww->cli->last_host(), ww->cli->last_port(), ww->cli->last_user())) {
                ww->cli->clear_reconnect();
                LOGS("timer: reconnect successful");
                notify(h, L"Overlay", L"Reconnected!");
            } else {
                LOGS("timer: reconnect failed, retrying in 3s");
                PostMessage(h, MSG_RECONNECT_ATTEMPT, 0, 0);
            }
        }
        return 0;
    case WM_DISPLAYCHANGE:
        if (ww) { ww->sw = GetSystemMetrics(SM_CXSCREEN); ww->sh = GetSystemMetrics(SM_CYSCREEN); }
        return 0;
    case WM_DESTROY: return 0;
    }
    return DefWindowProc(h, m, w, l);
}

}