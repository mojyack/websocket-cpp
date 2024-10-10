#pragma once
#include <functional>
#include <span>
#include <vector>

#include "common.hpp"
#include "impl.hpp"

namespace ws::server {
struct SessionDataInitializer {
    virtual auto alloc(lws* wsi) -> void* = 0;
    virtual auto free(void* ptr) -> void  = 0;

    virtual ~SessionDataInitializer() {}
};

using OnDataReceived = void(lws* wsi, std::span<const std::byte> payload);

enum class State {
    Connected,
    Destroyed,
};

struct ContextParams {
    const char*     protocol;
    const char*     cert        = nullptr;
    const char*     private_key = nullptr;
    int             port;
    KeepAliveParams keepalive = {};
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

    ~Context();
};

auto wsi_to_userdata(lws* wsi) -> void*;
} // namespace ws::server
