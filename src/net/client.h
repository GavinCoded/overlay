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
    cb_t cb;

    client_t();
    ~client_t();
    bool go(const std::string &h, int p, const std::string &u);
    void stop();
    void send(const std::string &t);
    bool is_open() { return connected; }
    void set_cb(cb_t c) { cb = std::move(c); }

private:
    void read();
    bool raw(const std::string &d);
    std::string host;
    int port;
    std::string user;
};

}