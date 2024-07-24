#pragma once
#include <functional>
#include <span>
#include <vector>

#include "impl.hpp"

namespace ws::client {
using OnDataReceived = void(std::span<const std::byte> payload);

enum class State {
    Initialized,
    Connected,
    Destroyed,
};

enum class SSLLevel {
    Secure,
    Unsecure,
    NoSSL,
};

struct Context {
    // set by library
    impl::AutoLWSContext   context;
    lws*                   wsi;
    std::vector<std::byte> buffer;
    State                  state;

    // set by user
    std::function<OnDataReceived> handler;

    // debug flags
    bool verbose      = false;
    bool dump_packets = false;

    auto init(const char* address, int port, const char* path, const char* protocol, const char* bind_address, SSLLevel ssl_level) -> bool;
    auto process() -> bool;
    auto shutdown() -> void;
};
} // namespace ws::client

