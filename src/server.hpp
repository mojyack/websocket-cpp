#pragma once
#include "server-common.hpp"

namespace ws::server {
using OnDataReceived = void(lws* wsi, std::span<const std::byte> payload);

struct Context : ContextCommon {
    // public
    std::function<OnDataReceived>           handler;
    std::unique_ptr<SessionDataInitializer> session_data_initer;

    auto init(const ContextParams& params) -> bool;
    auto send(lws* wsi, std::span<const std::byte> payload) -> bool;

    ~Context();
};

auto wsi_to_userdata(lws* wsi) -> void*;
} // namespace ws::server
