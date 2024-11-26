#include <libwebsockets.h>

#include "client.hpp"
#include "misc.hpp"
#include "util/logger.hpp"

#define CUTIL_MACROS_PRINT_FUNC logger.error
#include "macros/assert.hpp"

namespace ws::client {
namespace {
auto logger = Logger("ws");

auto ssl_level_to_flags(const SSLLevel level) -> int {
    switch(level) {
    case SSLLevel::Enable:
        return LCCSCF_USE_SSL;
    case SSLLevel::TrustSelfSigned:
        return LCCSCF_USE_SSL | LCCSCF_ALLOW_SELFSIGNED | LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK;
    default:
        return 0;
    }
}

auto callback(lws* wsi, lws_callback_reasons reason, void* const /*user*/, void* const in, const size_t len) -> int {
    const auto ctx = std::bit_cast<Context*>(lws_context_user(lws_get_context(wsi)));
    logger.debug("reason=", reason);

    switch(reason) {
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
        logger.debug("connection established");
        ctx->state = State::Connected;
        return 0;
    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        logger.warn("connection error");
        ctx->state = State::Destroyed;
        return -1;
    case LWS_CALLBACK_CLIENT_CLOSED:
        logger.debug("connection closed");
        ctx->state = State::Destroyed;
        return -1;
    case LWS_CALLBACK_CLIENT_RECEIVE: {
        if(ctx->dump_packets) {
            line_print(">>> ", len, " bytes:");
            dump_hex({(std::byte*)in, len});
        }
        const auto payload = impl::append_payload(wsi, ctx->receive_buffer, in, len);
        if(payload.empty()) {
            return 0;
        }
        if(ctx->handler) {
            ctx->handler(payload);
        }
        ctx->receive_buffer.clear();
        return 0;
    } break;
    case LWS_CALLBACK_CLIENT_WRITEABLE: {
        ensure(impl::send_all_of_send_buffers(ctx->send_buffers, wsi, ctx->text), "failed to send buffers");
        return 0;
    } break;
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
    const auto protocols = std::array<lws_protocols, 2>{{
        {
            .name           = params.protocol,
            .callback       = callback,
            .rx_buffer_size = 0x1000 * 1,
            .tx_packet_size = 0x1000 * 1,
        },
        {},
    }};

    const auto context_creation_info = lws_context_creation_info{
        .protocols              = protocols.data(),
        .port                   = CONTEXT_PORT_NO_LISTEN,
        .client_ssl_ca_filepath = params.cert,
        .ka_time                = params.keepalive.time,
        .ka_probes              = params.keepalive.probes,
        .ka_interval            = params.keepalive.interval,
        .gid                    = gid_t(-1),
        .uid                    = uid_t(-1),
        .options                = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT,
        .user                   = this,
    };
    context.reset(lws_create_context(&context_creation_info));
    ensure(context.get() != NULL);

    const auto host                = build_string(params.address, ":", params.port);
    const auto client_connect_info = lws_client_connect_info{
        .context                   = context.get(),
        .address                   = params.address,
        .port                      = params.port,
        .ssl_connection            = ssl_level_to_flags(params.ssl_level),
        .path                      = params.path,
        .host                      = host.data(),
        .protocol                  = params.protocol,
        .ietf_version_or_minus_one = -1,
        .iface                     = params.bind_address,
    };
    wsi = lws_client_connect_via_info(&client_connect_info);
    ensure(wsi != NULL);

    // wait for connection
    state = State::Initialized;
    while(state == State::Initialized) {
        lws_service(context.get(), 50);
    }
    return state == State::Connected;
}

auto Context::process() -> bool {
    lws_service(context.get(), 0);
    return state == State::Connected;
}

auto Context::send(const std::span<const std::byte> payload) -> bool {
    if(dump_packets) {
        line_print("<<< ", payload.size(), " bytes:");
        dump_hex(payload);
    }
    impl::push_to_send_buffers(send_buffers, payload);
    lws_callback_on_writable(wsi);
    lws_cancel_service_pt(wsi);
    return true;
}

auto Context::shutdown() -> void {
    state = State::Destroyed;
    lws_cancel_service(context.get());
}
} // namespace ws::client
