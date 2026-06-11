# Application logging

serves:
  - ../20-stories/connectivity.md
  - ../20-stories/updates.md
  - ../20-stories/feeding.md

## Purpose

All application and adapter diagnostic output on UART0 uses a single
`app_log` module for curated milestones and CLI responses. SDK syslog remains
enabled at `info` (`MTK_DEBUG_LEVEL=info`) so Wi-Fi/lwip stack lines still
appear alongside `app_log`. Optional secondary sinks (remote telnet mirror)
attach without changing call sites.

## Line format

Every log line on UART0:

```
[HH:MM:SS.mmm] [tag] message\r\n
```

| Field | Rule |
|-------|------|
| Timestamp | Elapsed time since boot: hours, minutes, seconds, milliseconds — zero-padded (`%02u:%02u:%02u.%03u`) |
| Time source | FreeRTOS tick (`xTaskGetTickCount` × tick period) on device; host tests use the fake tick clock |
| Tag | Lowercase module name in `[brackets]` — see [Tags](#tags) |
| Terminator | CRLF (`\r\n`) |

CLI handler responses use the same format with tag `cli` (see
[uart-console.md](uart-console.md)).

## Tags

Fixed lowercase strings. Callers pass the tag string to each `app_log_*` call.

| Tag | Owner |
|-----|-------|
| `cli` | MiniCLI handlers and `*_print_fail` helpers |
| `wifi` | `wifi_adapter`, `wifi_sta` error paths |
| `mqtt` | `mqtt_adapter`, `mqtt_client` |
| `motor` | `motor_ctrl` jam / anti-jam diagnostics |
| `ota` | `ota_adapter`, `ota_client` |
| `provision` | Captive portal and provisioning flow |
| `app` | `app.c`, `app_task` lifecycle, button bring-up UART |
| `hopper` | `hopper_input` level transitions |
| `i2c` | `i2c_bus_adapter` |
| `gpio` | `gpio_expander_adapter`, `aw9523_irq_adapter` |

New tags require spec review before use.

## Levels

| Level | Compile-time symbol | Use |
|-------|---------------------|-----|
| `debug` | `APP_LOG_LEVEL_DEBUG` | Verbose bench / OTA chunk progress |
| `info` | `APP_LOG_LEVEL_INFO` | Connection milestones, CLI success lines |
| `warn` | `APP_LOG_LEVEL_WARN` | Recoverable anomalies |
| `error` | `APP_LOG_LEVEL_ERROR` | Connect failures, driver errors |

Ordering: `debug` < `info` < `warn` < `error`.

`APP_LOG_LEVEL` in `firmware/GCC/feature.mk` selects the minimum level compiled
in (`debug` < `info` < `warn` < `error`). Messages below the threshold are
stripped at compile time — no runtime filter.

Default: `APP_LOG_LEVEL=debug`. A future production build profile may default
to `info` (drop `debug`-only chatter at build time).

## SDK policy

| Setting | Value |
|---------|-------|
| `MTK_DEBUG_LEVEL` | `info` — SDK syslog task active; Wi-Fi/lwip milestones may duplicate `app_log` |
| `MTK_MQTT_DEBUG_ENABLE` | `n` — MQTT stack yield/read spam off |

Do not enable additional SDK debug flags without spec review.

## Sinks

| Sink | When |
|------|------|
| UART0 via `log_write()` (board `io_def` DMA console, 115200 8N1) | After `bsp_io_def_uart_init()` in `prvSetupHardware()` |
| Optional mirror | Registered via `app_log_set_sink`; invoked after UART write |

`app_log_init()` does not configure UART — it only resets mirror-sink state.
Device output shares the MiniCLI `printf` path (`log_write` in `io_def.c`).
`app_log_uart_boot.patch` calls `app_log_init()` from `system_init()` after
display boot; it must not call `hal_uart_deinit()` (that breaks the console DMA
setup installed earlier in `prvSetupHardware()`).

On host unit tests (`HOST_TEST`), the default sink is `stdout` (no HAL).

## Wi-Fi milestones (tag `wifi`)

Canonical UART lines (message body only; full line includes timestamp prefix):

| Event | Line |
|-------|------|
| Before association | `connecting to "<ssid>"` |
| STA ready | `STA ready, IP <dotted-quad>` |
| Connect API failure | `connect failed` |
| Missing credentials | `no valid credentials in NVDM` |
| Disconnect (optional) | `STA disconnected` — deferred; do not register a second `WIFI_EVENT_IOT_DISCONNECTED` handler (overwrites SDK `wifi_lwip_helper` netif/DHCP cleanup) |

Curated lines are emitted from `wifi_sta` via `app_log`. SDK syslog may also
print link/DHCP lines (e.g. `DHCP got IP:…`) when `MTK_DEBUG_LEVEL=info`.

`wifi_sta` blocks on `lwip_net_ready()` after `wifi_port` connect, then reads
the STA IP. `EVT_WIFI_STA_CONNECTING` is posted from `wifi_sta_request_connect`
before the connect task runs.

## MQTT milestones (tag `mqtt`)

| Event | Line |
|-------|------|
| Connect attempt | `connecting to <host>:<port>` (once per burst) |
| Connected | `connected` |
| Connect failure | `connect failed` (once per burst) |

Per-message RX logging is **not** emitted on UART (see
[uart-console.md](uart-console.md) § MQTT connect).

## Motor diagnostics (tag `motor`)

Jam and anti-jam UART lines match [jam-detection.md](jam-detection.md) § UART
bring-up logging, with the unified timestamp prefix and `[motor]` tag.

## OTA diagnostics (tag `ota`)

Bench OTA scripts under `tools/ota/` grep UART captures for tagged `ota` lines
(message body only; full line includes timestamp prefix):

| Event | Line |
|-------|------|
| Command received | `accepted` |
| Download armed | `download started` |
| Per-chunk progress | `NN% (downloaded/total) heap=… min=…` |
| Download finished | `download complete bytes=…` |
| HTTP phase start | `mqtt down, http start` |

MQTT suspend/resume during OTA uses tag `mqtt` (`suspended for ota`,
`resumed after ota`). OTA bench `wait-mqtt-online.sh` treats `[mqtt] connected`
as boot-ready when UART is available.

## API (application code)

```c
void app_log_init(void);
void app_log_debug(const char *tag, const char *fmt, ...);
void app_log_info(const char *tag, const char *fmt, ...);
void app_log_warn(const char *tag, const char *fmt, ...);
void app_log_error(const char *tag, const char *fmt, ...);
void app_log_set_sink(void (*fn)(const char *buf, size_t len, void *ctx), void *ctx);
void app_log_clear_sink(void);
```

No `printf` or `#include "syslog.h"` in new or migrated `firmware/src/` code.

Optional convenience macros: `APP_LOG_D`, `APP_LOG_I`, `APP_LOG_W`, `APP_LOG_E`.
