#pragma once
#include "ov/win.h"

namespace ov {

LRESULT CALLBACK wnd_proc(HWND h, UINT m, WPARAM w, LPARAM l);
LRESULT CALLBACK hook_proc(int code, WPARAM wp, LPARAM lp);

}