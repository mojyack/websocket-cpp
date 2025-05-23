#include <libwebsockets.h>

#include "macros/logger.hpp"
#include "misc.hpp"
#include "server.hpp"

#define CUTIL_MACROS_PRINT_FUNC(...) LOG_ERROR(logger, __VA_ARGS__)
#include "macros/assert.hpp"

namespace ws::server {
namespace {
struct SessionData {
    void*             user_data = nullptr;
    impl::SendBuffers send_buffers;
};

auto logger = Logger("ws");

auto protocol_callback(lws* const wsi, const lws_callback_reasons reason, void* const user, void* const in, const size_t len) -> int {
    const auto ctx = std::bit_cast<Context*>(lws_context_user(lws_get_context(wsi)));
    LOG_DEBUG(logger, "reason={}", std::to_underlying(reason));

    switch(reason) {
    case LWS_CALLBACK_RECEIVE: {
        if(ctx->dump_packets) {
            PRINT("received {} bytes:", len);
            dump_hex({(std::byte*)in, len});
        }
        const auto payload = impl::append_payload(wsi, ctx->receive_buffer, in, len);
        if(payload.empty()) {
            return 0;
        }
        if(ctx->handler) {
            ctx->handler(wsi, payload);
        }
        ctx->receive_buffer.clear();
        return 0;
    }
    case LWS_CALLBACK_SERVER_WRITEABLE: {
        const auto base = (SessionData*)user;
        ensure(impl::send_one_from_send_buffer(base->send_buffers, wsi), "failed to send buffers");
        return 0;
    }
    case LWS_CALLBACK_ESTABLISHED: {
        const auto base = (SessionData*)user;
        new(base) SessionData();
        if(ctx->session_data_initer) {
            base->user_data = ctx->session_data_initer->alloc(wsi);
        }
        return 0;
    }
    case LWS_CALLBACK_CLOSED: {
        const auto base = (SessionData*)user;
        if(base->user_data != nullptr) {
            ctx->session_data_initer->free(base->user_data);
        }
        base->~SessionData();
        return 0;
    }
    case LWS_CALLBACK_EVENT_WAIT_CANCELLED:
        return ctx->state == State::Destroyed ? -1 : 0;
    case LWS_CALLBACK_WSI_DESTROY:
        lws_cancel_service_pt(wsi);
        return 0;
    default:
        return 0;
    }
}
} // namespace

auto Context::init(const ContextParams& params) -> bool {
    return init_protocol(params, sizeof(SessionData), (void*)protocol_callback);
}

auto Context::send(Client* const client, const std::span<const std::byte> payload) -> bool {
    if(dump_packets) {
        PRINT("sending {} bytes of binary:", payload.size());
        dump_hex(payload);
    }
    const auto base = (SessionData*)lws_wsi_user(client);
    impl::push_to_send_buffers_and_cancel_service(base->send_buffers, payload, client);
    return true;
}

auto Context::send(Client* const client, std::string_view payload) -> bool {
    if(dump_packets) {
        PRINT("sending {} bytes of text:", payload.size());
        std::println("{}", payload);
    }
    const auto base = (SessionData*)lws_wsi_user(client);
    impl::push_to_send_buffers_and_cancel_service(base->send_buffers, payload, client);
    return true;
}

Context::~Context() {
    // delete context explicitly before destructing other fields
    // because lws_context_destroy may call protocol_callback
    context.reset();
}

auto client_to_userdata(Client* const session) -> void* {
    return ((SessionData*)lws_wsi_user(session))->user_data;
}
} // namespace ws::server
