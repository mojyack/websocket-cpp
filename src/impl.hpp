#pragma once
#include <span>
#include <vector>

#include "macros/autoptr.hpp"

#define CUTIL_NS ws::impl
#include "util/writers-reader-buffer.hpp"
#undef CUTIL_NS

// libwebsockets
extern "C" {
struct lws_context;
struct lws;
auto lws_context_destroy(lws_context* context) -> void;
}

namespace ws::impl {
struct OutgoingPacket {
    std::vector<std::byte> data;
    bool                   text;
};
declare_autoptr(LWSContext, lws_context, lws_context_destroy);
using SendBuffers = WritersReaderBuffer<OutgoingPacket>;

auto append(std::vector<std::byte>& vec, void* in, size_t len) -> void;
auto append_payload(lws* wsi, std::vector<std::byte>& buffer, void* const in, const size_t len) -> std::span<std::byte>;
auto push_to_send_buffers_and_cancel_service(SendBuffers& send_buffers, std::span<const std::byte> payload, lws* wsi) -> void;
auto push_to_send_buffers_and_cancel_service(SendBuffers& send_buffers, std::string_view payload, lws* wsi) -> void;
auto send_all_of_send_buffers(SendBuffers& send_buffers, lws* wsi) -> bool;
} // namespace ws::impl
