#pragma once
#include <string>
#include <deque>

namespace ov {

struct msg_t {
    std::wstring sender;
    std::wstring text;
    bool sys;
};

struct chat_t {
    std::deque<msg_t> msgs;
    std::wstring input;
    int cursor;
    bool cursor_on;

    chat_t();
    void add(const std::wstring &s, const std::wstring &t, bool sys);
    void write(wchar_t ch);
    void back();
    void esc();
};

}