#pragma once
#include <windows.h>
#include <string>
#include <mutex>
#include "chat/box.h"
#include "net/client.h"
#include "vk/inst.h"
#include "vk/device.h"
#include "vk/gpu.h"
#include "vk/rp.h"
#include "vk/pipe.h"
#include "vk/font.h"
#include "vk/draw.h"

constexpr UINT MSG_TRAY = WM_APP + 1;
constexpr UINT MSG_RECONNECT = WM_APP + 5;
constexpr UINT MSG_RENDER = WM_APP + 10;
constexpr UINT MSG_AUTO_OPEN = WM_APP + 11;
constexpr UINT MSG_ENTER = WM_USER + 1;
constexpr UINT MSG_ESC = WM_USER + 2;
constexpr UINT MSG_BACK = WM_USER + 3;

constexpr UINT MENU_RECONNECT = 1001;
constexpr UINT MENU_EXIT = 1002;
constexpr UINT MENU_PAUSE = 1003;

constexpr UINT_PTR TIMER_CURSOR = 2;
constexpr UINT_PTR TIMER_CLOSE = 3;

namespace ov {

struct win_t {
    HWND hw;
    HINSTANCE hi;
    std::mutex mx;
    int cw, ch;
    bool open;
    bool cd;
    bool ntf;
    HHOOK hook;
    bool paused;
    std::wstring uname;
    std::wstring srv;
    std::wstring kn;
    DWORD kvk;
    chat_t chat;
    client_t *cli;
    int sw, sh;

    vk::inst_t inst;
    vk::device_t dev;
    vk::gpu_t gpu;
    vk::rp_t rp;
    vk::pipe_t pipe;
    vk::font_t font;
    vk::draw_t draw;

    HDC mem_dc;
    HBITMAP bmp;
    void *bits;
    HBITMAP old_bmp;
    HFONT font_handle;

    win_t();
    ~win_t();
    bool make(HINSTANCE i, const std::wstring &u, DWORD k,
        const std::wstring &kname, const std::wstring &sa);
    void set_cli(client_t *c) { cli = c; }
    void add_msg(const std::wstring &s, const std::wstring &t, bool sys);
    void open_ch();
    void close_ch();
    void tog_ch();
};

}