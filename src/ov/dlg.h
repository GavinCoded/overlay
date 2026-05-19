#pragma once
#include <windows.h>

namespace ov {

struct client_t;
struct win_t;

struct dlg_ctx {
    client_t *cli;
    win_t *win;
};

}

INT_PTR CALLBACK dlg_proc(HWND hw, UINT m, WPARAM w, LPARAM l);
