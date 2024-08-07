#include <thread>

#include "client.hpp"
#include "macros/assert.hpp"
#include "misc.hpp"
#include "server.hpp"
#include "util/span.hpp"

namespace {
auto run() -> bool {
    ws::set_log_level(0xff);
    auto server    = ws::server::Context();
    server.handler = [&server](lws* wsi, std::span<const std::byte> payload) -> void {
        print("server received message: ", std::string_view((char*)payload.data(), payload.size()));
        server.send(wsi, to_span("ack"));
    };
    server.verbose      = true;
    server.dump_packets = true;
    assert_b(server.init({
        .protocol    = "message",
        .cert        = "files/localhost.cert",
        .private_key = "files/localhost.key",
        .port        = 8080,
    }));
    auto server_thread = std::thread([&server]() -> void {
        while(server.state == ws::server::State::Connected) {
            server.process();
        }
    });

    auto client         = ws::client::Context();
    client.verbose      = true;
    client.dump_packets = true;
    client.handler      = [](std::span<const std::byte> payload) -> void {
        print("client received message: ", std::string_view((char*)payload.data(), payload.size()));
    };
    assert_b(client.init({
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
    server.shutdown();
    server_thread.join();

    return true;
}
} // namespace

auto main() -> int {
    return run() ? 0 : 1;
}
