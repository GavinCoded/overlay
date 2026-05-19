#include "chat/box.h"

namespace ov {

chat_t::chat_t() : cursor(0), cursor_on(true) {}

void chat_t::add(const std::wstring &s, const std::wstring &t, bool sys) {
    if (msgs.size() >= 100) msgs.pop_front();
    msgs.push_back({s, t, sys});
}

void chat_t::write(wchar_t ch) {
    if (ch == L'\r' || ch == L'\n') return;
    input.insert((size_t)cursor, 1, ch);
    cursor++;
}

void chat_t::back() {
    if (cursor <= 0) return;
    input.erase((size_t)cursor - 1, 1);
    cursor--;
}

void chat_t::esc() { input.clear(); cursor = 0; }

}