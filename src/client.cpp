#include <libwebsockets.h>

#include "client.hpp"
#include "macros/assert.hpp"
#include "misc.hpp"

namespace ws::client {
namespace {
auto ssl_level_to_flags(const SSLLevel level) -> int {
    switch(level) {
    case SSLLevel::Secure:
        return LCCSCF_USE_SSL | LCCSCF_ALLOW_SELFSIGNED | LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK;
        break;
    case SSLLevel::Unsecure:
        return LCCSCF_USE_SSL;
    default:
        return 0;
    }
}

auto callback(lws* wsi, lws_callback_reasons reason, void* const /*user*/, void* const in, const size_t len) -> int {
    const auto ctx = std::bit_cast<Context*>(lws_context_user(lws_get_context(wsi)));
    if(ctx->verbose) {
        PRINT("reason: ", reason);
    }

    switch(reason) {
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
        if(ctx->verbose) {
            PRINT("connection established ");
        }
        ctx->state = State::Connected;
        return 0;
    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        if(ctx->verbose) {
            PRINT("connection error");
        }
        ctx->state = State::Destroyed;
        return -1;
    case LWS_CALLBACK_CLOSED:
    case LWS_CALLBACK_CLIENT_CLOSED:
        if(ctx->verbose) {
            PRINT("connection close");
        }
        ctx->state = State::Destroyed;
        return -1;
    case LWS_CALLBACK_CLIENT_RECEIVE: {
        if(ctx->dump_packets) {
            auto str = std::string_view(std::bit_cast<char*>(in));
            PRINT(">>> ", str);
        }
        const auto payload = impl::append_payload(wsi, ctx->buffer, in, len);
        if(payload.empty()) {
            return 0;
        }
        if(ctx->handler) {
            ctx->handler(payload);
        }
        ctx->buffer.clear();
        return 0;
    } break;
    case LWS_CALLBACK_EVENT_WAIT_CANCELLED:
        return ctx->state == State::Destroyed ? -1 : 0;
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
        .protocols = protocols.data(),
        .port      = CONTEXT_PORT_NO_LISTEN,
        .gid       = gid_t(-1),
        .uid       = uid_t(-1),
        .options   = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT,
        .user      = this,
    };
    context.reset(lws_create_context(&context_creation_info));
    assert_b(context.get() != NULL);

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
    assert_b(wsi != NULL);

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
        auto str = std::string_view(std::bit_cast<char*>(payload.data()), payload.size());
        PRINT("<<<", str);
    }
    assert_b(size_t(write_back(wsi, payload.data(), payload.size())) == payload.size());
    return true;
}

auto Context::shutdown() -> void {
    state = State::Destroyed;
    lws_cancel_service(context.get());
}
} // namespace ws::client
