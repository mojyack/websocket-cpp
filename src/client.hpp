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
    Enable,
    TrustSelfSigned,
    Disable,
};

struct ContextParams {
    const char* address;
    const char* path;
    const char* protocol;
    const char* cert         = nullptr;
    const char* bind_address = nullptr;
    int         port;
    SSLLevel    ssl_level;
    uint16_t    connection_check_interval   = 60; // interval between ping packets in seconds
    uint16_t    connection_invalidate_delay = 10; // delay between last pong packet and hangup in seconds
};

struct Context {
    // set by library
    impl::AutoLWSContext   context;
    lws*                   wsi;
    std::vector<std::byte> receive_buffer;
    impl::SendBuffers      send_buffers;
    State                  state;

    // set by user
    std::function<OnDataReceived> handler;

    // debug flags
    bool verbose      = false;
    bool dump_packets = false;

    auto init(const ContextParams& params) -> bool;
    auto process() -> bool;
    auto send(std::span<const std::byte> payload) -> bool;
    auto shutdown() -> void;
};
} // namespace ws::client

