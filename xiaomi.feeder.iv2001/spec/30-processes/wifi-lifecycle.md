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
| 1 | `set_credentials` — `wifi_config_set_ssid` / `wifi_config_set_wpa_psk_key` (or open security); **no** `reload_setting` yet |
| 2 | `radio_up` — `wifi_config_set_radio(1)` and start lwIP STA netif if stopped |
| 3 | `arm_connect` — `wifi_config_reload_setting()` (one scan/connect pass); returns immediately |
| 4 | `wait_ready` — block up to `[tune]` 60 s on SDK `PORT_SECURE` and DHCP semaphores (`lwip_net_ready_timed`) |

Credentials must be staged before `set_radio(1)` — enabling the radio with an
empty SSID profile can stall scan/connect for ~30 s (Airoha IoT SDK for RTOS
Wi-Fi Developer's Guide §2.1, configuration APIs before `reload_setting`).
`wifi_session_connect` uses the four-step order above; provisioning may call
`connect` (`set_credentials` + `arm_connect`) when the radio is already up.

`wait_ready` must not poll `wifi_connection_get_link_status()` alone — during
bank-B boot the link stays down for ~30 s while the N9 is idle, then events
arrive in quick succession. `[probe]`

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
stack-init STA cache wipe in `wifi_adapter_stack_init()` (see
[wifi-lifecycle.md](wifi-lifecycle.md) § Bank-B boot delay).

Provisioning flows that reference this sequence: [provisioning-flow.md](provisioning-flow.md#sdk-sta-profile-invalidate).

## UART CLI

`wifi disconnect` tears down the STA session via `wifi_sta_request_disconnect()`
on the connect task. See [uart-console.md](uart-console.md#wifi-disconnect).

## Bank-B boot delay

Booting while flash bank B is active (OTA apply, `bank switch`, or cold start
with the control block pointing at B) shows a ~30 s N9 idle gap before
`PORT_SECURE` and DHCP — **not** tied to OTA download or A/B slot health.
Bank-A boots reach `STA ready` in ~1–2 s on the same bench. `[probe]`

Firmware has no bank-specific Wi-Fi source branches; A and B images differ only
by link base (`0x08012000` vs `0x08100000`). Mitigations already in tree that
do **not** remove the gap:

- `connsys_force_n9_reset.patch` (N9 SW reset at every boot)
- `wifi_adapter_wipe_sta_caches()` before `wifi_init()`
- `lwip_net_ready_timed` instead of link-status polling
- Credential-before-radio connect order (`set_credentials` → `radio_up` →
  `arm_connect`) `[probe]` 2026-06-16

Typical bank-B UART timeline after `bank switch`: `connecting` and
`reload_setting` within ~70 ms of scheduler start; `[fw_event] start connect`
from `__seek_and_connect` ~39 s later; `STA ready` ~42 s. WPA msg 3 may log
MIC/sanity errors before the handshake completes. `[probe]` 2026-06-16

`wifi_boot_connect_timeout_ms()` returns `[tune]` 60 s when bank B is active
(headroom for the N9 ROM `seek_and_connect` window); bank A uses the same
default until bench proves a shorter budget is safe.

See [ota-flow.md](ota-flow.md) § Known limitation for UART symptoms and
slot-health confirm timing.

## Acceptance

`[tune]` Bench: `wifi show` → `wifi connect` → `wifi disconnect` →
`wifi connect` round-trip over UART succeeds without reboot.

`[tune]` Bank B: power-cycle or `bank switch` to B → `STA ready` within 60 s
(Wi-Fi icon may stay off for ~30 s while `wait_ready` blocks).
