#include "log.h"
#include <time.h>

namespace ov {

bool g_debug = IsDebuggerPresent();
log g_log;

log::log() : console(false), fp(nullptr) {
    if (!g_debug) return;
    AllocConsole();
    SetConsoleTitle(L"");
    freopen_s(&fp, "CONOUT$", "w", stdout);
    console = true;
    auto h = GetStdHandle(STD_OUTPUT_HANDLE);
  

}

log::~log() {
    if (console && fp) {
        print("debug console closing\n");
        fclose(fp);
        FreeConsole();
    }
}

void log::print(const char *fmt, ...) {
    if (!fp) return;
    va_list args;
    va_start(args, fmt);
    vfprintf(fp, fmt, args);
    va_end(args);
    fflush(fp);
}

}