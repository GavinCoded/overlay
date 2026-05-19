#include "ov/dlg.h"
#include "ov/reg.h"
#include "resource.h"
#include "net/client.h"
#include "ov/win.h"
#include "log.h"
#include <string>
#include <shobjidl.h>

namespace {

struct kb_entry { wchar_t name[16]; DWORD vk; };
kb_entry kbs[] = {
    {L"T", 'T'}, {L"Y", 'Y'}, {L"U", 'U'}, {L"J", 'J'}, {L"K", 'K'},
    {L"F1", VK_F1}, {L"F2", VK_F2}, {L"F3", VK_F3}, {L"F4", VK_F4},
    {L"F5", VK_F5}, {L"F6", VK_F6}, {L"F7", VK_F7}, {L"F8", VK_F8},
    {L"/", VK_OEM_2}, {L"Insert", VK_INSERT},
};

int kb_idx(DWORD vk) {
    for (auto &k : kbs)
        if (k.vk == vk) return (int)(&k - kbs);
    return 0;
}

DWORD idx_vk(int i) {
    return (i < 0 || i >= (int)std::size(kbs)) ? 'T' : kbs[i].vk;
}

}

INT_PTR CALLBACK dlg_proc(HWND hw, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_INITDIALOG: {
        SetWindowLongPtr(hw, GWLP_USERDATA, l);
        auto sa = ov::reg_read(L"ServerAddress", L"127.0.0.1:8080");
        auto su = ov::reg_read(L"Username", L"Player");
        auto sk = ov::reg_read_dw(L"Keybind", 'T');
        SetDlgItemText(hw, IDC_SERVER_ADDR, sa.c_str());
        SetDlgItemText(hw, IDC_USERNAME, su.c_str());
        auto cb = GetDlgItem(hw, IDC_KEYBIND);
        for (auto &k : kbs) SendMessage(cb, CB_ADDSTRING, 0, (LPARAM)k.name);
        SendMessage(cb, CB_SETCURSEL, kb_idx(sk), 0);
        return TRUE;
    }
    case WM_COMMAND:
        if (w == IDOK) {
            wchar_t ab[256] = {}, ub[64] = {};
            GetDlgItemText(hw, IDC_SERVER_ADDR, ab, 256);
            GetDlgItemText(hw, IDC_USERNAME, ub, 64);
            auto addr = std::wstring(ab);
            auto uname = std::wstring(ub);
            if (addr.empty() || uname.empty()) {
                MessageBox(hw, L"enter server and username", L"error", MB_ICONWARNING);
                return TRUE;
            }
            auto col = addr.find(L':');
            if (col == std::wstring::npos) {
                MessageBox(hw, L"use host:port format", L"error", MB_ICONWARNING);
                return TRUE;
            }
            auto host = addr.substr(0, col);
            int port = 0;
            try { port = std::stoi(addr.substr(col + 1)); }
            catch (...) { MessageBox(hw, L"invalid port", L"error", MB_ICONWARNING); return TRUE; }

            auto cb = GetDlgItem(hw, IDC_KEYBIND);
            auto sel = SendMessage(cb, CB_GETCURSEL, 0, 0);
            auto kvk = idx_vk((int)sel);
            wchar_t kn[16] = {};
            SendMessage(cb, CB_GETLBTEXT, sel, (LPARAM)kn);

            auto w2a = [](const std::wstring &w) {
                auto len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
                std::string s(len, 0);
                WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, s.data(), len, nullptr, nullptr);
                s.resize(len - 1); return s;
            };

            auto ctx = (ov::dlg_ctx *)GetWindowLongPtr(hw, GWLP_USERDATA);

            ctx->cli->set_cb([ctx](const std::string &s, const std::string &t, bool sys) {
                LOG("callback: sys=%d sender='%s' text='%s'", sys, s.c_str(), t.c_str());
                std::wstring ws(s.begin(), s.end()), wt(t.begin(), t.end());
                if (ctx->win) {
                    ctx->win->add_msg(ws, wt, sys);
                    if (sys && ctx->win->hw) {
                        auto u = ctx->cli->last_user();
                        if (t.find("disconnected") != std::string::npos) {
                            LOG("callback: detected 'disconnected' -> posting MSG_RECONNECT_ATTEMPT");
                            PostMessage(ctx->win->hw, MSG_RECONNECT_ATTEMPT, 0, 0);
                        } else if (t.find("left the server") != std::string::npos && t.find(u) != std::string::npos) {
                            LOG("callback: detected '%s left the server' -> posting MSG_RECONNECT_ATTEMPT", u.c_str());
                            PostMessage(ctx->win->hw, MSG_RECONNECT_ATTEMPT, 0, 0);
                        }
                    }
                }
            });

            if (ctx->cli->go(w2a(host), port, w2a(uname))) {
                ov::reg_write(L"ServerAddress", addr);
                ov::reg_write(L"Username", uname);
                ov::reg_write_dw(L"Keybind", kvk);
                if (!ctx->win->make(GetModuleHandle(nullptr), uname, kvk, kn, addr)) {
                    MessageBox(hw, L"failed to create overlay (vulkan missing or shitty gpu).", L"error", MB_ICONERROR);
                    ctx->cli->stop();
                    EndDialog(hw, IDCANCEL);
                    return TRUE;
                }
                ctx->win->set_cli(ctx->cli);
                EndDialog(hw, IDOK);
            } else {
                MessageBox(hw, L"failed to connect", L"error", MB_ICONERROR);
            }
            return TRUE;
        }
        if (w == IDCANCEL) { EndDialog(hw, IDCANCEL); return TRUE; }
        break;
    }
    return FALSE;
}
