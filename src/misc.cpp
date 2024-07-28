#include <libwebsockets.h>

#include "misc.hpp"

namespace ws {
auto set_log_level(const uint8_t bitmap) -> void {
    lws_set_log_level(bitmap, NULL);
}

auto write_back(lws* const wsi, const void* const data, size_t size) -> int {
    auto buffer       = std::vector<std::byte>(LWS_SEND_BUFFER_PRE_PADDING + size + LWS_SEND_BUFFER_POST_PADDING);
    auto payload_head = buffer.data() + LWS_SEND_BUFFER_PRE_PADDING;
    memcpy(payload_head, data, size);
    return lws_write(wsi, std::bit_cast<unsigned char*>(payload_head), size, LWS_WRITE_TEXT);
}

auto write_back_str(lws* const wsi, const std::string_view str) -> int {
    return write_back(wsi, str.data(), str.size());
}
} // namespace ws
