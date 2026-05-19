#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <winsock2.h>
#include <ws2tcpip.h>

namespace ov {

using cb_t = std::function<void(const std::string &, const std::string &, bool)>;

struct client_t {
    SOCKET s;
    std::thread th;
    std::atomic<bool> connected;
    std::atomic<bool> running;
    std::atomic<bool> shutting_down;
    cb_t cb;

    client_t();
    ~client_t();
    bool go(const std::string &h, int p, const std::string &u);
    void stop();
    void send(const std::string &t);
    bool is_open() { return connected; }
    void set_cb(cb_t c) { cb = std::move(c); }
    bool should_reconnect() { return reconnect; }
    void clear_reconnect() { reconnect = false; }
    std::string last_host() { return host; }
    int last_port() { return port; }
    std::string last_user() { return user; }

private:
    void read();
    bool raw(const std::string &d);
    std::string host;
    int port = 0;
    std::string user;
    std::atomic<bool> reconnect;
};

}