#include <libwebsockets.h>

#include "misc.hpp"

namespace ws {
auto set_log_level(const uint8_t bitmap) -> void {
    lws_set_log_level(bitmap, NULL);
}

auto dump_hex(const std::span<const std::byte> data) -> void {
    for(auto i = 0u; i < data.size();) {
        printf("%04X  ", i);
        auto ascii = std::array<char, 9>();
        for(auto c = 0; c < 8; c += 1, i += 1) {
            if(i >= data.size()) {
                break;
            }

            printf("%02X ", uint8_t(data[i]));
            ascii[c] = isprint(int(data[i])) != 0 ? char(data[i]) : '.';
        }
        printf("  |%s|\n", ascii.data());
    }
}

auto write_back(lws* const wsi, const void* const data, size_t size) -> int {
    auto buffer       = std::vector<std::byte>(LWS_SEND_BUFFER_PRE_PADDING + size + LWS_SEND_BUFFER_POST_PADDING);
    auto payload_head = buffer.data() + LWS_SEND_BUFFER_PRE_PADDING;
    memcpy(payload_head, data, size);
    return lws_write(wsi, std::bit_cast<unsigned char*>(payload_head), size, LWS_WRITE_TEXT);
}
} // namespace ws
