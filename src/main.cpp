#include <windows.h>
#include <shobjidl.h>
#include "resource.h"
#include "ov/dlg.h"
#include "ov/win.h"
#include "ov/tray.h"
#include "net/client.h"

int WINAPI WinMain(HINSTANCE inst, HINSTANCE, LPSTR, int) {
    SetCurrentProcessExplicitAppUserModelID(L"Overlay");

    while (true) {
        ov::client_t cli;
        ov::win_t win;
        ov::dlg_ctx ctx = {&cli, &win};

        if (DialogBoxParam(inst, MAKEINTRESOURCE(IDD_CONNECT_DIALOG),
            nullptr, dlg_proc, (LPARAM)&ctx) != IDOK)
            return 0;

        ov::tray_t tray;
        tray.add(win.hw);
        if (win.ntf) PostMessage(win.hw, MSG_RENDER, 0, 0);

        MSG msg = {};
        BOOL ret;
        while ((ret = GetMessage(&msg, nullptr, 0, 0)) != 0) {
            if (ret == -1) break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        tray.del();
        win.close_ch();
        if (msg.wParam != 1) break;
    }

    return 0;
}
