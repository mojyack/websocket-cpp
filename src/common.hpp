#pragma once

namespace ws {
struct KeepAliveParams {
    int time     = 60; // 0: no keepalive
    int probes   = 3;
    int interval = 5;
};
} // namespace ws
