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

auto push_to_send_buffers(SendBuffers& send_buffers, const std::span<const std::byte> payload) -> void {
    auto buf  = std::vector<std::byte>(LWS_SEND_BUFFER_PRE_PADDING + payload.size() + LWS_SEND_BUFFER_POST_PADDING);
    auto head = buf.data() + LWS_SEND_BUFFER_PRE_PADDING;
    memcpy(head, payload.data(), payload.size());
    send_buffers.push(std::move(buf));
}

auto send_all_of_send_buffers(SendBuffers& send_buffers, lws* const wsi, const bool text) -> bool {
    for(const auto& buf : send_buffers.swap()) {
        const auto head = buf.data() + LWS_SEND_BUFFER_PRE_PADDING;
        const auto size = buf.size() - LWS_SEND_BUFFER_PRE_PADDING - LWS_SEND_BUFFER_POST_PADDING;
        if(lws_write(wsi, std::bit_cast<unsigned char*>(head), size, text ? LWS_WRITE_TEXT : LWS_WRITE_BINARY) != int(size)) {
            return false;
        }
    }
    return true;
}
} // namespace ws::impl
