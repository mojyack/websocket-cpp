#pragma once
#include "impl.hpp"

namespace ws {
auto set_log_level(uint8_t bitmap) -> void;
auto dump_hex(std::span<const std::byte> data) -> void;
auto write_back(lws* const wsi, const void* const data, size_t size) -> int;
auto write_back_str(lws* const wsi, const std::string_view str) -> int;
} // namespace ws

