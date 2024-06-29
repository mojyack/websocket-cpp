#pragma once
#include <span>
#include <vector>

#include "macros/autoptr.hpp"

// libwebsockets
extern "C" {
struct lws_context;
struct lws;
auto lws_context_destroy(lws_context* context) -> void;
}

namespace ws::impl {
declare_autoptr(LWSContext, lws_context, lws_context_destroy);

auto append(std::vector<std::byte>& vec, void* in, size_t len) -> void;
auto append_payload(lws* wsi, std::vector<std::byte>& buffer, void* const in, const size_t len) -> std::span<std::byte>;
} // namespace ws::impl
