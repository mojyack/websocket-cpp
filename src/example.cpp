#include <thread>

#include "client.hpp"
#include "macros/assert.hpp"
#include "misc.hpp"
#include "server.hpp"
#include "util/assert.hpp"

auto run() -> bool {
    ws::set_log_level(0xff);
    auto server    = ws::server::Context();
    server.handler = [](lws* wsi, std::span<const std::byte> payload) -> void {
        print("server received message: ", std::string_view((char*)payload.data(), payload.size()));
        ws::write_back_str(wsi, "ack");
    };
    server.verbose      = true;
    server.dump_packets = true;
    assert_b(server.init(8080, "message"));
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
    assert_b(client.init("localhost", 8080, "/", "message", ws::client::SSLLevel::NoSSL));
    auto client_thread = std::thread([&client]() -> void {
        while(client.state == ws::client::State::Connected) {
            client.process();
        }
    });
    for(auto i = 0; i < 5; i += 1) {
        ws::write_back_str(client.wsi, build_string("hello, ", i));
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    client.shutdown();
    client_thread.join();
    server.shutdown();
    server_thread.join();

    return true;
}

auto main() -> int {
    return run() ? 0 : 1;
}
