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

auto write_back(lws* const wsi, const void* const data, size_t size) -> int {
    auto buffer       = std::vector<std::byte>(LWS_SEND_BUFFER_PRE_PADDING + size + LWS_SEND_BUFFER_POST_PADDING);
    auto payload_head = buffer.data() + LWS_SEND_BUFFER_PRE_PADDING;
    memcpy(payload_head, data, size);
    return lws_write(wsi, std::bit_cast<unsigned char*>(payload_head), size, LWS_WRITE_TEXT);
}
} // namespace ws::impl
