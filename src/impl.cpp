#include <libwebsockets.h>

#include "impl.hpp"
#include "util/span.hpp"

namespace ws::impl {
namespace {
auto push_to_send_buffers(SendBuffers& send_buffers, const std::span<const std::byte> payload, const bool text) -> void {
    auto data = std::vector<std::byte>(LWS_SEND_BUFFER_PRE_PADDING + payload.size() + LWS_SEND_BUFFER_POST_PADDING);
    auto head = data.data() + LWS_SEND_BUFFER_PRE_PADDING;
    memcpy(head, payload.data(), payload.size());
    send_buffers.push({std::move(data), text});
}
} // namespace

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

auto push_to_send_buffers_and_cancel_service(SendBuffers& send_buffers, const std::span<const std::byte> payload, lws* const wsi) -> void {
    push_to_send_buffers(send_buffers, payload, false);
    lws_callback_on_writable(wsi);
    lws_cancel_service_pt(wsi);
}

auto push_to_send_buffers_and_cancel_service(SendBuffers& send_buffers, const std::string_view payload, lws* const wsi) -> void {
    push_to_send_buffers(send_buffers, to_span(payload), true);
    lws_callback_on_writable(wsi);
    lws_cancel_service_pt(wsi);
}

auto send_all_of_send_buffers(SendBuffers& send_buffers, lws* const wsi) -> bool {
    for(const auto& packet : send_buffers.swap()) {
        const auto head = packet.data.data() + LWS_SEND_BUFFER_PRE_PADDING;
        const auto size = packet.data.size() - LWS_SEND_BUFFER_PRE_PADDING - LWS_SEND_BUFFER_POST_PADDING;
        const auto ret  = lws_write(wsi, std::bit_cast<unsigned char*>(head), size, packet.text ? LWS_WRITE_TEXT : LWS_WRITE_BINARY);
        if(ret < int(size)) {
            return false;
        }
    }
    return true;
}
} // namespace ws::impl
