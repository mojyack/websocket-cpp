#include <thread>

#include "client.hpp"
#include "macros/assert.hpp"
#include "misc.hpp"
#include "server.hpp"
#include "util/span.hpp"

namespace {
namespace server {
struct SessionData {
    int    a;
    size_t b;
    bool   c;
};

struct SessionDataInitializer : ws::server::SessionDataInitializer {
    auto alloc(ws::server::Client* /*client*/) -> void* override {
        return new SessionData{.a = -1, .b = 0, .c = true};
    }

    auto free(void* const ptr) -> void override {
        delete(SessionData*)ptr;
    }
};

struct Server {
    ws::server::Context websocket_context;
    std::thread         worker_thread;

    auto init() -> bool;
    auto stop() -> void;
};

auto Server::init() -> bool {
    websocket_context.handler = [this](ws::server::Client* client, std::span<const std::byte> payload) -> void {
        auto& session = *std::bit_cast<SessionData*>(ws::server::client_to_userdata(client));
        print("server received message: ", std::string_view((char*)payload.data(), payload.size()));
        print("session: a=", session.a, " b=", session.b, " c=", session.c);
        websocket_context.send(client, to_span("ack"));
    };
    websocket_context.session_data_initer.reset(new SessionDataInitializer());
    websocket_context.verbose      = true;
    websocket_context.dump_packets = true;
    ensure(websocket_context.init({
        .protocol    = "message",
        .cert        = "files/localhost.cert",
        .private_key = "files/localhost.key",
        .port        = 8080,
    }));

    worker_thread = std::thread([this]() -> void {
        while(websocket_context.state == ws::server::State::Connected) {
            websocket_context.process();
        }
    });

    return true;
}

auto Server::stop() -> void {
    websocket_context.shutdown();
    worker_thread.join();
}
} // namespace server

auto run() -> bool {
    ws::set_log_level(0xff);
    auto server = server::Server();
    ensure(server.init());

    auto client         = ws::client::Context();
    client.dump_packets = true;
    client.handler      = [](std::span<const std::byte> payload) -> void {
        print("client received message: ", std::string_view((char*)payload.data(), payload.size()));
    };
    ensure(client.init({
        .address   = "localhost",
        .path      = "/",
        .protocol  = "message",
        .cert      = "files/localhost.cert",
        .port      = 8080,
        .ssl_level = ws::client::SSLLevel::TrustSelfSigned,
    }));
    auto client_thread = std::thread([&client]() -> void {
        while(client.state == ws::client::State::Connected) {
            client.process();
        }
    });
    for(auto i = 0; i < 5; i += 1) {
        const auto str = build_string("hello, ", i);
        client.send(to_span(str));
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    client.shutdown();
    client_thread.join();
    server.stop();

    return true;
}
} // namespace

auto main() -> int {
    return run() ? 0 : 1;
}
