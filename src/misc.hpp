#pragma once
#include <cstdint>
#include <span>

namespace ws {
auto set_log_level(uint8_t bitmap) -> void;
auto dump_hex(std::span<const std::byte> data) -> void;
} // namespace ws

