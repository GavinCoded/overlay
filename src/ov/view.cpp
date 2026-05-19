#include "ov/view.h"
#include "ov/tray.h"
#include "log.h"
#include "rgba.h"
#include <cstdint>
#include <string>

namespace ov {

static std::wstring truncate(HDC dc, const std::wstring &text, int maxw) {
    SIZE sz = {};
    GetTextExtentPoint32W(dc, text.c_str(), (int)text.size(), &sz);
    if (sz.cx <= maxw) return text;
    for (size_t i = text.size(); i > 0; i--) {
        auto t = text.substr(0, i) + L"...";
        GetTextExtentPoint32W(dc, t.c_str(), (int)t.size(), &sz);
        if (sz.cx <= maxw) return t;
    }
    return L"...";
}

void render(win_t &w) {
    std::lock_guard<std::mutex> lock(w.mx);
    auto &msgs = w.chat.msgs;
    int lh = 26, pd = 10, ih = 28;
    int mab = w.ch - ih - 12;
    int maxv = (mab - pd - 8) / lh;
    int cnt = (int)msgs.size();
    int vis = cnt > maxv ? maxv : cnt;
    int start = cnt - vis;
    int y = mab - lh - (vis - 1) * lh;

    w.draw.begin(w.dev, w.rp, w.pipe);
    w.draw.end(w.dev, w.rp, w.gpu);
    w.draw.copy_to(w.gpu.mapped, w.bits, w.cw * 4);

    auto dc = w.mem_dc;
    SelectObject(dc, w.font_handle);
    SetBkMode(dc, TRANSPARENT);

    int bg_t = y - 4;
    int bg_b = w.ch;
    if (bg_t < 0) bg_t = 0;
    if (bg_b > w.ch) bg_b = w.ch;

    auto px = (uint32_t *)w.bits;
    for (int yy = bg_t; yy < bg_b; yy++)
        for (int xx = 0; xx < w.cw; xx++)
            px[yy * w.cw + xx] = 0xA0000000;

    int maxw = w.cw - pd * 2;
    for (int i = start; i < cnt; i++) {
        auto &msg = msgs[(size_t)i];
        if (y + lh > w.ch - pd - ih) break;
        if (msg.sys) {
            auto t = L"[system] " + truncate(dc, msg.text, maxw);
            SetTextColor(dc, RGB(255, 217, 89));
            TextOutW(dc, pd, y, t.c_str(), (int)t.size());
        } else {
            auto disp = msg.sender + L": ";
            SIZE sz = {};
            GetTextExtentPoint32W(dc, disp.c_str(), (int)disp.size(), &sz);
            int tw = maxw - sz.cx;
            auto t = truncate(dc, msg.text, tw > 50 ? tw : maxw);
            SetTextColor(dc, RGB(128, 204, 255));
            TextOutW(dc, pd, y, disp.c_str(), (int)disp.size());
            SetTextColor(dc, RGB(255, 255, 255));
            TextOutW(dc, pd + sz.cx, y, t.c_str(), (int)t.size());
        }
        y += lh;
    }

    if (!w.cd) {
        y = w.ch - pd - ih;
        auto prompt = std::wstring(L"> ") + w.chat.input;
        if (w.chat.cursor_on) {
            auto cp = w.chat.cursor;
            prompt = prompt.substr(0, 2 + (size_t)cp) + L"_" + prompt.substr(2 + (size_t)cp);
        }
        SetTextColor(dc, RGB(255, 255, 255));
        TextOutW(dc, pd, y, prompt.c_str(), (int)prompt.size());
    }

    for (int yy = bg_t; yy < bg_b; yy++)
        for (int xx = 0; xx < w.cw; xx++) {
            auto &p = px[yy * w.cw + xx];
            if (p & 0x00FFFFFF) p = (p & 0x00FFFFFF) | 0xFF000000;
        }

    auto blend = BLENDFUNCTION{};
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;
    SIZE sz = {w.cw, w.ch};
    POINT pt0 = {0, 0};
    POINT pt1 = {w.sw - w.cw - 20, w.sh - w.ch - 20};
    auto sdc = GetDC(nullptr);
    UpdateLayeredWindow(w.hw, sdc, &pt1, &sz, w.mem_dc, &pt0, 0, &blend, ULW_ALPHA);
    ReleaseDC(nullptr, sdc);
}

}
