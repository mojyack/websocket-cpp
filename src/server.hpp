#pragma once
#include <functional>
#include <span>
#include <vector>

#include "impl.hpp"

namespace ws::server {
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

struct Context {
    // set by library
    impl::AutoLWSContext   context;
    std::vector<std::byte> buffer;
    State                  state;

    // set by user
    std::function<OnDataReceived>           handler;
    std::unique_ptr<SessionDataInitializer> session_data_initer;

    // debug flags
    bool verbose      = false;
    bool dump_packets = false;

    auto init(int port, const char* protocol) -> bool;
    auto process() -> bool;
    auto send(lws* wsi, std::span<const std::byte> payload) -> bool;
    auto shutdown() -> void;
};

auto wsi_to_userdata(lws* wsi) -> void*;
} // namespace ws::server
