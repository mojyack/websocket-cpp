#include <libwebsockets.h>

#include "macros/unwrap.hpp"
#include "misc.hpp"
#include "server.hpp"

namespace ws::server {
namespace {
auto http_callback(lws* const wsi, const lws_callback_reasons reason, void* const /*user*/, void* const /*in*/, const size_t /*len*/) -> int {
    switch(reason) {
    case LWS_CALLBACK_HTTP:
        return lws_serve_http_file(wsi, "index.html", "text/html", NULL, 0);
    default:
        return 0;
    }
}

auto protocol_callback(lws* const wsi, const lws_callback_reasons reason, void* const user, void* const in, const size_t len) -> int {
    const auto ctx = std::bit_cast<Context*>(lws_context_user(lws_get_context(wsi)));
    if(ctx->verbose) {
        PRINT("reason: ", reason);
    }

    switch(reason) {
    case LWS_CALLBACK_RECEIVE: {
        if(ctx->dump_packets) {
            PRINT(">>> ", len, " bytes:");
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
    case LWS_CALLBACK_SERVER_WRITEABLE:
        if(!impl::send_all_of_send_buffers(ctx->send_buffers, wsi)) {
            PRINT("failed to send buffers");
            return -1;
        }
        return 0;
    case LWS_CALLBACK_ESTABLISHED:
        if(ctx->session_data_initer) {
            ctx->session_data_initer->init(user, wsi);
        }
        return 0;
    case LWS_CALLBACK_CLOSED:
        if(ctx->session_data_initer) {
            ctx->session_data_initer->deinit(user);
        }
        return 0;
    case LWS_CALLBACK_EVENT_WAIT_CANCELLED:
        return ctx->state == State::Destroyed ? -1 : 0;
    default:
        return 0;
    }
}
} // namespace

auto Context::init(const ContextParams& params) -> bool {
    const auto protocols = std::array<lws_protocols, 3>{{
        {
            .name                  = "http-only",
            .callback              = http_callback,
            .per_session_data_size = 0,
            .rx_buffer_size        = 0x1000 * 1,
            .tx_packet_size        = 0x1000 * 1,
        },
        {
            .name                  = params.protocol,
            .callback              = protocol_callback,
            .per_session_data_size = session_data_initer ? session_data_initer->get_size() : 0,
            .rx_buffer_size        = 0x1000 * 1,
            .tx_packet_size        = 0x1000 * 1,
        },
        {},
    }};

    const auto context_creation_info = lws_context_creation_info{
        .protocols                = protocols.data(),
        .port                     = params.port,
        .ssl_cert_filepath        = params.cert,
        .ssl_private_key_filepath = params.private_key,
        .ka_time                  = 60,
        .ka_probes                = 3,
        .ka_interval              = 5,
        .gid                      = gid_t(-1),
        .uid                      = uid_t(-1),
        .options                  = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT,
        .user                     = this,
    };
    unwrap_pb_mut(lws_context, lws_create_context(&context_creation_info));
    context.reset(&lws_context);

    state = State::Connected;

    return true;
}

auto Context::process() -> bool {
    lws_service(context.get(), 0);
    return true;
}

auto Context::send(lws* const wsi, std::span<const std::byte> payload) -> bool {
    if(dump_packets) {
        PRINT("<<< ", payload.size(), " bytes:");
        dump_hex(payload);
    }
    impl::push_to_send_buffers(send_buffers, payload);
    lws_callback_on_writable(wsi);
    return true;
}

auto Context::shutdown() -> void {
    state = State::Destroyed;
    lws_cancel_service(context.get());
}

auto wsi_to_userdata(lws* wsi) -> void* {
    return lws_wsi_user(wsi);
}
} // namespace ws::server
