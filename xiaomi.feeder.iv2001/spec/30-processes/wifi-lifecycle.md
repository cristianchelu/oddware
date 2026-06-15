# Wi-Fi lifecycle

serves:
  - ../20-stories/connectivity.md
  - ../20-stories/provisioning.md

## Boot

| Step | Behavior |
|------|----------|
| Stack init | `wifi_adapter_stack_init()` runs once during boot; STA-only mode; `sta_auto_connect=0` `[design]` |
| Auto-connect | Disabled at stack init — association starts only via `wifi_sta_request_connect` or provisioning test-connect |
| Stored SSID | When NVDM `wifi/ssid` is non-empty at boot, queue connect equivalent (see [uart-console.md](uart-console.md#boot-behavior-wi-fi)) |

LinkIt Wi-Fi Developers Guide §2.1.5: set `sta_auto_connect=0`, then
`wifi_config_reload_setting()` to connect. In-band Wi-Fi APIs run only from
tasks after the scheduler starts (§2.1.6).

## Connect sequence

Association and DHCP run on the Wi-Fi connect task (not the CLI task).

| Step | Action |
|------|--------|
| 1 | `radio_up` — enable radio and start lwIP STA netif if stopped |
| 2 | `connect` — set SSID/PSK (or open security), `wifi_config_reload_setting()`; returns immediately |
| 3 | `wait_ready` — block up to `[tune]` 30 s for `WIFI_EVENT_IOT_PORT_SECURE` (4-way complete) and DHCP |

`PORT_SECURE` means the 4-way handshake is complete; DHCP follows (Dev Guide
Table 5). On success the connect task posts `EVT_WIFI_STA_READY` with the
assigned IPv4 address.

Connect APIs run only after the scheduler is running and after
`WIFI_EVENT_IOT_INIT_COMPLETE` when the adapter registers that gate. If the
stack is not ready, `wait_ready` fails fast with an error (Dev Guide Table 3).

## Disconnect sequence

Full STA teardown is idempotent — safe to call when already down.

| Step | Action |
|------|----------|
| 1 | `lwip_net_stop(STA)` — release DHCP while link semantics remain valid |
| 2 | `wifi_connection_disconnect_ap()` |
| 3 | `wifi_config_set_radio(0)` — radio off |

**Order:** `disconnect_ap` before `set_radio(0)`. LinkIt SDK v4 Release Notes
(MT7682): calling `set_radio(0)` before AP disconnect can cause sleep failure.

Sleep and peripheral power-down paths call `wifi_port.disconnect()` (see
[wfci-bus-arbitration.md](wfci-bus-arbitration.md#sleep-and-wake)).

## SDK STA profile invalidate

Canonical home for clearing the LinkIt HAL `STA/*` NVDM profile during
provisioning abort, factory reset, and AP entry. App NVDM `wifi/*` is untouched.

| Step | Action |
|------|--------|
| 1 | `wifi_connection_stop_scan()` |
| 2 | `wifi_connection_disconnect_ap()` |
| 3 | Clear NVDM `STA/SsidLen`, `STA/Ssid`, `STA/WpaPskLen`, `STA/WpaPsk` |
| 4 | `wifi_config_reload_setting()` |

Does **not** clear `STA/PMK_INFO` or `common/StaFastLink` — those belong to
post-OTA boot cache wipe in the adapter (out of scope for this process).

Provisioning flows that reference this sequence: [provisioning-flow.md](provisioning-flow.md#sdk-sta-profile-invalidate).

## UART CLI

`wifi disconnect` tears down the STA session via `wifi_sta_request_disconnect()`
on the connect task. See [uart-console.md](uart-console.md#wifi-disconnect).

## Acceptance

`[tune]` Bench: `wifi show` → `wifi connect` → `wifi disconnect` →
`wifi connect` round-trip over UART succeeds without reboot.
