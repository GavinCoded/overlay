#include "net/client.h"
#include "log.h"
#include <cstring>

namespace ov {

client_t::client_t() : s(INVALID_SOCKET),
    connected(false), running(false) { WSADATA w; WSAStartup(MAKEWORD(2,2), &w); }

client_t::~client_t() { stop(); WSACleanup(); }

bool client_t::go(const std::string &h, int p, const std::string &u) {
    LOG("connecting to %s:%d as '%s'", h.c_str(), p, u.c_str());
    host = h; port = p; user = u;

    LOGS("  creating socket");
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) { LOGS("  socket failed"); return false; }

    u_long mode = 1;
    ioctlsocket(s, FIONBIO, &mode);
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons((u_short)p);
    LOG("  resolving %s", h.c_str());
    addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo *ai = nullptr;
    if (getaddrinfo(h.c_str(), nullptr, &hints, &ai) != 0 || !ai) {
        LOGS("  dns resolution failed"); closesocket(s); return false;
    }
    addr.sin_addr = ((sockaddr_in *)ai->ai_addr)->sin_addr;
    freeaddrinfo(ai);

    LOGS("  connecting (5s timeout)");
    ::connect(s, (sockaddr *)&addr, sizeof(addr));
    fd_set fd; FD_ZERO(&fd); FD_SET(s, &fd);
    TIMEVAL tv = {5, 0};
    if (select(0, nullptr, &fd, nullptr, &tv) <= 0) {
        LOGS("  connection timed out"); closesocket(s); return false;
    }
    mode = 0; ioctlsocket(s, FIONBIO, &mode);
    LOGS("  socket connected");

    connected = true; running = true;
    th = std::thread(&client_t::read, this);
    raw("NICK|" + u);
    LOG("connected to %s:%d as %s", h.c_str(), p, u.c_str());
    return true;
}

void client_t::stop() {
    connected = false; running = false;
    if (s != INVALID_SOCKET) {
        shutdown(s, SD_BOTH); closesocket(s); s = INVALID_SOCKET;
    }
    if (th.joinable()) th.join();
}

bool client_t::raw(const std::string &d) {
    if (!connected || s == INVALID_SOCKET) return false;
    auto line = d + "\n";
    auto n = ::send(s, line.c_str(), (int)line.size(), 0);
    return n > 0;
}

void client_t::send(const std::string &t) {
    LOGS("send msg");
    raw("MSG|" + t);
}

void client_t::read() {
    std::string buf;
    char tmp[4096];
    while (running && connected) {
        auto n = recv(s, tmp, sizeof(tmp) - 1, 0);
        if (n <= 0) {
            connected = false;
            if (cb) cb("", "disconnected from server", true);
            break;
        }
        tmp[n] = 0; buf += tmp;
        size_t pos;
        while ((pos = buf.find('\n')) != std::string::npos) {
            auto line = buf.substr(0, pos);
            buf.erase(0, pos + 1);
            if (line.empty()) continue;
            auto p1 = line.find('|');
            auto type = line.substr(0, p1);
            if (type == "MSG") {
                auto rest = line.substr(p1 + 1);
                auto p2 = rest.find('|');
                if (p2 != std::string::npos && cb) {
                    cb(rest.substr(0, p2), rest.substr(p2 + 1), false);
                    LOG("msg: %s: %s", rest.substr(0, p2).c_str(), rest.substr(p2 + 1).c_str());
                }
            } else if (type == "SYS" && cb) {
                cb("", line.substr(p1 + 1), true);
            } else if (type == "OK" && cb) {
                cb("", "connected to chat server", true);
            }
        }
    }
}

}