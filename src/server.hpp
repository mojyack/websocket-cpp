#pragma once
#include <functional>
#include <span>
#include <vector>

#include "impl.hpp"

namespace ws::server {
struct SessionData {
    impl::SendBuffers send_buffers;
};

struct SessionDataInitializer {
    virtual auto get_size() -> size_t              = 0;
    virtual auto init(void* ptr, lws* wsi) -> void = 0;
    virtual auto deinit(void* ptr) -> void         = 0;

    virtual ~SessionDataInitializer() {}
};

using OnDataReceived = void(lws* wsi, std::span<const std::byte> payload);

enum class State {
    Connected,
    Destroyed,
};

struct ContextParams {
    const char* protocol;
    const char* cert        = nullptr;
    const char* private_key = nullptr;
    int         port;
    uint16_t    connection_check_interval   = 60; // interval between ping packets in seconds
    uint16_t    connection_invalidate_delay = 10; // delay between last pong packet and hangup in seconds
};

struct Context {
    // set by library
    impl::AutoLWSContext   context;
    std::vector<std::byte> receive_buffer;
    State                  state;

    // set by user
    std::function<OnDataReceived>           handler;
    std::unique_ptr<SessionDataInitializer> session_data_initer;

    // debug flags
    bool verbose      = false;
    bool dump_packets = false;

    auto init(const ContextParams& params) -> bool;
    auto process() -> bool;
    auto send(lws* wsi, std::span<const std::byte> payload) -> bool;
    auto shutdown() -> void;
};

auto wsi_to_userdata(lws* wsi) -> void*;
} // namespace ws::server
