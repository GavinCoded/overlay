#pragma once
#include <windows.h>
#include <cstdio>

namespace ov {

struct log {
    bool console;
    FILE *fp;

    log();
    ~log();
    void print(const char *fmt, ...);
};

extern log g_log;
extern bool g_debug;

}

#define LOG(fmt, ...) do { if (ov::g_debug) ov::g_log.print(fmt "\n", __VA_ARGS__); } while(0)
#define LOGS(s) do { if (ov::g_debug) ov::g_log.print("%s\n", s); } while(0)