#pragma once
#include "server-common.hpp"

namespace ws::server {
using Client                 = lws;
using SessionDataInitializer = SessionDataInitializerCommon<Client>;
using OnDataReceived         = OnDataReceivedCommon<Client>;

struct Context : ContextCommon {
    // public
    std::function<OnDataReceived>           handler;
    std::unique_ptr<SessionDataInitializer> session_data_initer;

    auto init(const ContextParams& params) -> bool;
    auto send(Client* client, std::span<const std::byte> payload) -> bool;

    ~Context();
};

auto client_to_userdata(Client* client) -> void*;
} // namespace ws::server
