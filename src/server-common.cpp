#include <libwebsockets.h>

#include "server-common.hpp"
#include "util/logger.hpp"

#define CUTIL_MACROS_PRINT_FUNC logger.error
#include "macros/unwrap.hpp"

namespace ws::server {
namespace {
auto logger = Logger("ws");

auto http_callback(lws* const wsi, const lws_callback_reasons reason, void* const /*user*/, void* const /*in*/, const size_t /*len*/) -> int {
    switch(reason) {
    case LWS_CALLBACK_HTTP:
        return lws_serve_http_file(wsi, "index.html", "text/html", NULL, 0);
    default:
        return 0;
    }
}

} // namespace

auto ContextCommon::init_protocol(const ContextParams& params, const size_t session_data_size, const void* const session_callback) -> bool {
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
            .callback              = (lws_callback_function*)session_callback,
            .per_session_data_size = session_data_size,
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
        .ka_time                  = params.keepalive.time,
        .ka_probes                = params.keepalive.probes,
        .ka_interval              = params.keepalive.interval,
        .gid                      = gid_t(-1),
        .uid                      = uid_t(-1),
        .options                  = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT,
        .user                     = this,
    };
    unwrap_mut(lws_context, lws_create_context(&context_creation_info));
    context.reset(&lws_context);

    state = State::Connected;

    return true;
}

auto ContextCommon::process() -> bool {
    lws_service(context.get(), 0);
    return true;
}

auto ContextCommon::shutdown() -> void {
    state = State::Destroyed;
    lws_cancel_service(context.get());
}
} // namespace ws::server
