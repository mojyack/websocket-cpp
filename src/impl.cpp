#include <libwebsockets.h>

#include "impl.hpp"

namespace ws::impl {
auto append(std::vector<std::byte>& vec, void* const in, const size_t len) -> void {
    const auto ptr = std::bit_cast<std::byte*>(in);
    vec.insert(vec.end(), ptr, ptr + len);
}

auto append_payload(lws* wsi, std::vector<std::byte>& buffer, void* const in, const size_t len) -> std::span<std::byte> {
    const auto remaining = lws_remaining_packet_payload(wsi);
    const auto final     = lws_is_final_fragment(wsi);
    if(remaining != 0 || !final) {
        append(buffer, in, len);
        return {};
    }
    if(buffer.empty()) {
        return {std::bit_cast<std::byte*>(in), len};
    } else {
        append(buffer, in, len);
        return buffer;
    }
}
} // namespace ws::impl
