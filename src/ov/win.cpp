#include "ov/win.h"
#include "ov/proc.h"
#include "log.h"

namespace ov {

win_t *g_win = nullptr;

LRESULT CALLBACK hook_proc(int code, WPARAM wp, LPARAM lp) {
    if (code >= 0 && g_win && g_win->open && !g_win->cd) {
        auto kb = (KBDLLHOOKSTRUCT *)lp;
        if (wp == WM_KEYDOWN) {
            if (kb->vkCode == VK_RETURN) { PostMessage(g_win->hw, MSG_ENTER, 0, 0); return 1; }
            if (kb->vkCode == VK_ESCAPE) { PostMessage(g_win->hw, MSG_ESC, 0, 0); return 1; }
            if (kb->vkCode == VK_BACK) { PostMessage(g_win->hw, MSG_BACK, 0, 0); return 1; }
            wchar_t buf[4] = {}; BYTE ks[256] = {}; GetKeyboardState(ks);
            auto len = ToUnicode(kb->vkCode, kb->scanCode, ks, buf, 4, 0);
            if (len > 0) {
                for (int i = 0; i < len; i++)
                    if (buf[i] >= 32) PostMessage(g_win->hw, WM_CHAR, buf[i], 0);
                return 1;
            }
            return 1;
        }
    }
    return (LRESULT)CallNextHookEx(nullptr, code, wp, lp);
}

win_t::win_t() : hw(nullptr), hi(nullptr), cw(700), ch(380), open(false),
    cd(false), ntf(false), hook(nullptr), paused(false), kvk('T'), cli(nullptr), sw(0), sh(0),
    mem_dc(nullptr), bmp(nullptr), bits(nullptr), old_bmp(nullptr) { g_win = this; }

win_t::~win_t() {
    if (mem_dc && old_bmp) SelectObject(mem_dc, old_bmp);
    if (bmp) DeleteObject(bmp);
    if (mem_dc) DeleteDC(mem_dc);
    if (hook) UnhookWindowsHookEx(hook);
    draw.destroy(dev); font.destroy(dev); pipe.destroy(dev);
    rp.destroy(dev); gpu.destroy(dev); dev.destroy(); inst.destroy();
    if (font_handle) DeleteObject(font_handle);
    if (hw) DestroyWindow(hw);
}

bool win_t::make(HINSTANCE i, const std::wstring &u, DWORD k,
    const std::wstring &kname, const std::wstring &sa)
{
    hi = i; uname = u; kvk = k; kn = kname; srv = sa;
    sw = GetSystemMetrics(SM_CXSCREEN); sh = GetSystemMetrics(SM_CYSCREEN);
    LOG("screen: %dx%d, window: %dx%d", sw, sh, cw, ch);
    LOGS("creating vulkan resources");
    if (!inst.create()) return false;
    if (!dev.create(inst.v)) return false;
    if (!gpu.create(dev, cw, ch)) return false;
    if (!font.create(dev)) return false;
    if (!rp.create(dev, gpu.view, cw, ch)) return false;
    if (!pipe.create(dev, rp.rp, font.view)) return false;
    if (!draw.create(dev, cw, ch)) return false;

    LOGS("creating overlay window");
    auto wc = WNDCLASSEX{};
    wc.cbSize = sizeof(WNDCLASSEX); wc.lpfnWndProc = wnd_proc;
    wc.hInstance = i; wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"OverlayWindow";
    RegisterClassEx(&wc);

    int x = sw - cw - 20, y = (sh - ch) / 2;
    hw = CreateWindowEx(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST
        | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        L"OverlayWindow", L"", WS_POPUP, x, y, cw, ch,
        nullptr, nullptr, i, nullptr);
    if (!hw) { LOGS("window creation failed"); return false; }
    SetWindowLongPtr(hw, GWLP_USERDATA, (LONG_PTR)this);
    LOG("hwnd: 0x%p", hw);

    LOGS("creating dib section");
    auto scrdc = GetDC(nullptr);
    auto bi = BITMAPV5HEADER{};
    bi.bV5Size = sizeof(BITMAPV5HEADER); bi.bV5Width = cw; bi.bV5Height = -ch;
    bi.bV5Planes = 1; bi.bV5BitCount = 32; bi.bV5Compression = BI_RGB;
    bmp = CreateDIBSection(scrdc, (BITMAPINFO *)&bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    mem_dc = CreateCompatibleDC(scrdc);
    old_bmp = (HBITMAP)SelectObject(mem_dc, bmp);
    ReleaseDC(nullptr, scrdc);
    LOG("dib: %dx%d at 0x%p", cw, ch, bits);

    RegisterHotKey(hw, 1, MOD_NOREPEAT, kvk);
    font_handle = CreateFontW(-20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    LOG("font created: 0x%p", font_handle);

    LOGS("window ready");
    return true;
}

void win_t::add_msg(const std::wstring &s, const std::wstring &t, bool sys) {
    { std::lock_guard<std::mutex> lock(mx); chat.add(s, t, sys); }
    if (sys && t.find(L"connected") != std::wstring::npos)
        ntf = true;
    if (hw) {
        if (open || ntf)
            PostMessage(hw, MSG_RENDER, 0, 0);
        if (!open && !sys)
            PostMessage(hw, MSG_AUTO_OPEN, 0, 0);
    }
}

void win_t::open_ch() {
    open = true; cd = false; chat.esc(); chat.cursor_on = true;
    KillTimer(hw, TIMER_CURSOR); UnregisterHotKey(hw, 1);
    hook = SetWindowsHookEx(WH_KEYBOARD_LL, hook_proc, hi, 0);
    SetTimer(hw, TIMER_CURSOR, 500, nullptr);
    ShowWindow(hw, SW_SHOW); PostMessage(hw, MSG_RENDER, 0, 0);
}

void win_t::close_ch() {
    open = false; cd = false;
    if (hook) { UnhookWindowsHookEx(hook); hook = nullptr; }
    KillTimer(hw, TIMER_CURSOR); KillTimer(hw, TIMER_CLOSE);
    if (!paused) RegisterHotKey(hw, 1, MOD_NOREPEAT, kvk);
    ShowWindow(hw, SW_HIDE);
}

void win_t::tog_ch() {
    if (open) {
        if (cd) { KillTimer(hw, TIMER_CLOSE); open_ch(); }
        else close_ch();
    } else {
        open_ch();
    }
}

}