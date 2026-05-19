#pragma once
#include <windows.h>

namespace ov {

struct tray_t {
    HWND hwnd;

    void add(HWND h);
    void del();
    void show(const wchar_t *title, const wchar_t *text);
};

}