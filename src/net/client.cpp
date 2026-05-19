#include "net/client.h"
#include "log.h"
#include <cstring>
#include <mstcpip.h>

namespace ov {

client_t::client_t() : s(INVALID_SOCKET),
    connected(false), running(false), shutting_down(false), reconnect(false) {
    WSADATA w;
    if (WSAStartup(MAKEWORD(2,2), &w) != 0) { s = INVALID_SOCKET; connected = false; running = false; }
}

client_t::~client_t() { stop(); WSACleanup(); }

bool client_t::go(const std::string &h, int p, const std::string &u) {
    LOG("connecting to %s:%d as '%s'", h.c_str(), p, u.c_str());
    host = h; port = p; user = u; reconnect = false; shutting_down = false;
    if (s != INVALID_SOCKET) { closesocket(s); s = INVALID_SOCKET; }

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
    auto sel = select(0, nullptr, &fd, nullptr, &tv);
    if (sel <= 0) {
        LOG("  connection failed: select returned %d (error %d)", sel, WSAGetLastError());
        closesocket(s); return false;
    }
    mode = 0; ioctlsocket(s, FIONBIO, &mode);
    LOGS("  socket connected");

    LOGS("  enabling tcp keepalive");
    BOOL keepalive = TRUE;
    if (setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (const char*)&keepalive, sizeof(keepalive)) != 0)
        LOG("  setsockopt SO_KEEPALIVE failed: %d", WSAGetLastError());
    tcp_keepalive kalive = {};
    kalive.onoff = TRUE;
    kalive.keepalivetime = 30000;
    kalive.keepaliveinterval = 10000;
    DWORD ret;
    if (WSAIoctl(s, SIO_KEEPALIVE_VALS, &kalive, sizeof(kalive), nullptr, 0, &ret, nullptr, nullptr) != 0)
        LOG("  WSAIoctl SIO_KEEPALIVE_VALS failed: %d", WSAGetLastError());
    else
        LOGS("  tcp keepalive enabled");

    connected = true; running = true;
    LOGS("  starting read thread");
    th = std::thread(&client_t::read, this);
    LOG("  sending NICK as '%s'", u.c_str());
    raw("NICK|" + u);
    LOG("connected to %s:%d as %s", h.c_str(), p, u.c_str());
    return true;
}

void client_t::stop() {
    LOGS("client stop: shutting down");
    shutting_down = true;
    connected = false; running = false;
    if (s != INVALID_SOCKET) {
        LOGS("client stop: closing socket");
        if (shutdown(s, SD_BOTH) != 0)
            LOG("  shutdown failed: %d", WSAGetLastError());
        if (closesocket(s) != 0)
            LOG("  closesocket failed: %d", WSAGetLastError());
        s = INVALID_SOCKET;
    }
    if (th.joinable()) { LOGS("client stop: joining read thread"); th.join(); }
    LOGS("client stop: done");
}

bool client_t::raw(const std::string &d) {
    if (!connected) { LOGS("raw send: not connected"); return false; }
    if (s == INVALID_SOCKET) { LOGS("raw send: invalid socket"); return false; }
    auto line = d + "\n";
    auto n = ::send(s, line.c_str(), (int)line.size(), 0);
    if (n <= 0) LOG("raw send failed (%d bytes, error %d)", n, WSAGetLastError());
    else if (n < (int)line.size()) LOG("raw send partial: %d of %zu bytes", n, line.size());
    return n > 0;
}

void client_t::send(const std::string &t) {
    LOG("send msg: %s", t.c_str());
    if (!raw("MSG|" + t))
        LOG("send msg: raw send returned false");
}

void client_t::read() {
    LOGS("read thread started");
    std::string buf;
    char tmp[4096];
    while (running && connected) {
        auto n = recv(s, tmp, sizeof(tmp) - 1, 0);
        if (n == 0) {
            LOGS("recv returned 0 (server closed connection gracefully)");
            connected = false;
            if (!shutting_down) {
                LOGS("read: initiating reconnect (not shutting down)");
                reconnect = true;
                if (cb) cb("", "disconnected from server", true);
            } else {
                LOGS("read: ignoring disconnect (shutting down)");
            }
            break;
        }
        if (n < 0) {
            auto err = WSAGetLastError();
            LOG("recv failed: error %d", err);
            connected = false;
            if (!shutting_down) {
                LOGS("read: initiating reconnect (not shutting down)");
                reconnect = true;
                if (cb) cb("", "disconnected from server", true);
            } else {
                LOGS("read: ignoring disconnect (shutting down)");
            }
            break;
        }
        tmp[n] = 0; buf += tmp;
        LOG("read: received %d bytes (buffer %zu)", n, buf.size());
        size_t pos;
        while ((pos = buf.find('\n')) != std::string::npos) {
            auto line = buf.substr(0, pos);
            buf.erase(0, pos + 1);
            if (line.empty()) { LOGS("read: skipping empty line"); continue; }
            auto p1 = line.find('|');
            auto type = line.substr(0, p1);
            LOG("read: parsed type=%s len=%zu", type.c_str(), line.size());
            if (type == "MSG") {
                auto rest = line.substr(p1 + 1);
                auto p2 = rest.find('|');
                if (p2 != std::string::npos && cb) {
                    cb(rest.substr(0, p2), rest.substr(p2 + 1), false);
                    LOG("msg: %s: %s", rest.substr(0, p2).c_str(), rest.substr(p2 + 1).c_str());
                }
            } else if (type == "SYS" && cb) {
                auto text = line.substr(p1 + 1);
                LOG("sys: %s", text.c_str());
                cb("", text, true);
            } else if (type == "OK" && cb) {
                LOGS("ok: connected to chat server");
                cb("", "connected to chat server", true);
            } else {
                LOG("read: unknown message type: %s", type.c_str());
            }
        }
    }
    LOGS("read thread exiting");
}

}