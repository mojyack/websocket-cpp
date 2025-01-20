#include <array>
#include <print>

#include <ctype.h>
#include <libwebsockets.h>

#include "misc.hpp"

namespace ws {
auto set_log_level(const uint8_t bitmap) -> void {
    lws_set_log_level(bitmap, NULL);
}

auto dump_hex(const std::span<const std::byte> data) -> void {
    for(auto i = 0uz; i < data.size();) {
        std::print("{:04X}", i);
        auto ascii = std::array<char, 16 + 1>();
        for(auto c = 0; c < 8 && i < data.size(); c += 1, i += 1) {
            std::print("{:02X} ", uint8_t(data[i]));
            ascii[c] = isprint(int(data[i])) != 0 ? char(data[i]) : '.';
        }
        std::print(" ");
        for(auto c = 8; c < 16 && i < data.size(); c += 1, i += 1) {
            std::print("{:02X} ", uint8_t(data[i]));
            ascii[c] = isprint(int(data[i])) != 0 ? char(data[i]) : '.';
        }
        std::println("  |{}|", ascii.data());
    }
}
} // namespace ws
