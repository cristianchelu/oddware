# UART console (development CLI)

serves:
  - ../20-stories/updates.md
  - ../20-stories/connectivity.md
  - ../20-stories/provisioning.md

## Transport

| Parameter | Value |
|-----------|-------|
| Interface | UART0 — GPIO21 (TX), GPIO22 (RX) |
| Baud | 115200 8N1 |
| Engine | LinkIt MiniCLI (multi-level commands, line history) `[design]` |

The console is always available in application firmware for bench bring-up,
bank inspection, and Wi-Fi credential entry before MQTT provisioning exists.
It is not a user-facing product interface. `[design]`

## Command tree

Top-level commands registered by application firmware:

```
bank show
bank switch
wifi show
wifi set ssid <name>
wifi set pass <password>
wifi connect
mqtt show
mqtt set host <hostname>
mqtt set port <port>
mqtt set user <username>
mqtt set pass <password>
mqtt set device_id <id>
mqtt connect
mqtt disconnect
display test
display fill <hex_byte>
display off
display number <0-999> [g|%]
display icon <name> on|off
display icon <name> blink <on_ms> <off_ms>
display icon <name> steady
display anim <ota|lock> [loop]
display anim stop
display brightness <1-4>
weigh power on|off
weigh read
weigh cal zero
weigh cal span
weigh cal status
config factory-reset
```

Command matching is case-sensitive. Extra arguments after the required
parameters are ignored by `wifi set` handlers.

## Wi-Fi credential rules

Canonical validation for `wifi/ssid` and `wifi/pass` NVDM keys (see
[config-store.md](config-store.md)). The same rules apply to captive-portal
submission in [provisioning-flow.md](provisioning-flow.md).

| Field | Rule | On violation |
|-------|------|--------------|
| SSID | Non-empty; length 1–32 bytes (802.11 SSID limit) | Reject with validation error |
| Password | Empty allowed (open network); if non-empty, length 8–63 bytes (WPA2-PSK passphrase range) | Reject with validation error |

`wifi_cred_is_stored` is true when NVDM `wifi/ssid` exists and is non-empty.
A stored SSID is sufficient to connect. The `wifi/pass` key is optional: if
missing or empty, the device treats the network as open (no PSK).

| `wifi/pass` in NVDM | Meaning |
|---------------------|---------|
| Key missing | Open network (no password required) |
| Empty string | Open network (explicit) |
| Non-empty string | WPA2-PSK passphrase (8–63 bytes) |

## `bank` commands

### `bank show`

Prints the active A/B application bank.

| Outcome | UART response |
|---------|---------------|
| Success | `active bank: A` or `active bank: B` followed by CRLF |

### `bank switch`

Toggles the dual-image active flag to the other bank and reboots immediately.

| Step | Action |
|------|--------|
| 1 | Flip active flag in the A/B control block |
| 2 | Print confirmation |
| 3 | Reboot whole system |

| Outcome | UART response |
|---------|---------------|
| Success | `bank switched — rebooting` then reboot |
| Failure | `bank switch failed` |

## `wifi` commands

Commands read and write NVDM group `wifi` (`ssid`, `pass`). They do not
start AP provisioning mode.

### `wifi show`

Displays stored Wi-Fi credentials from the app `wifi` NVDM namespace
(not the LinkIt SDK `STA` profile). Password is never printed in cleartext.

| Outcome | UART response |
|---------|---------------|
| SSID unset | `ssid: (unset)` |
| SSID set | `ssid: <value>` |
| SSID unset | `pass: (unset)` |
| SSID set, open network (`wifi/pass` missing or empty) | `pass: (open)` |
| SSID set, WPA2 passphrase stored | `pass: ********` |

### `wifi set ssid <name>`

Validates and writes `wifi/ssid` to NVDM. Does not connect.

| Outcome | UART response |
|---------|---------------|
| Success | `ssid saved` |
| Missing argument | `usage: wifi set ssid <name>` |
| Validation failure | `invalid ssid` |
| NVDM write failure | `nvdm write failed` |

### `wifi set pass <password>`

Validates and writes `wifi/pass` to NVDM. Does not connect. An empty
password is accepted (open network).

| Outcome | UART response |
|---------|---------------|
| Success | `password saved` |
| Missing argument | `usage: wifi set pass <password>` |
| Validation failure | `invalid password` |
| NVDM write failure | `nvdm write failed` |

### `wifi set` (invalid subcommand)

| Outcome | UART response |
|---------|---------------|
| Missing or unknown subcommand | `usage: wifi set ssid|pass <value>` |

### `wifi connect`

Loads credentials from NVDM, validates the pair, and starts STA
association with DHCP. Runs in the Wi-Fi connect task (not the CLI task).

| Precondition | Behavior |
|--------------|----------|
| `wifi/ssid` not stored | Reject without connecting |
| SSID stored (with or without `wifi/pass`) | Queue connect; associate; wait for DHCP |

| Outcome | UART response |
|---------|---------------|
| SSID not stored | `set ssid first` |
| Connect queued | `connecting...` |
| Connect already in progress | `connect already in progress` |

On successful association and DHCP, syslog prints (not necessarily inline
with the CLI prompt):

- `connecting to "<ssid>"`
- `DHCP got IP:<dotted-quad>` (SDK lwIP helper)
- `STA ready, IP <dotted-quad>`

| Failure | Log / behavior |
|---------|----------------|
| Missing or invalid NVDM credentials | `no valid credentials in NVDM` (connect task) |
| SDK connect API failure | `connect failed` (connect task) |

## Boot behavior (Wi-Fi)

| Condition | Action |
|-----------|--------|
| Boot | Initialize Wi-Fi stack in STA-only mode with auto-connect **disabled** |
| `wifi/ssid` stored in NVDM at boot | Automatically queue `wifi connect` equivalent |
| No stored SSID | Enter AP provisioning mode (see [provisioning-flow.md](provisioning-flow.md)) |

## MQTT broker rules

Canonical validation for `mqtt/*` NVDM keys (see
[config-store.md](config-store.md)). The same rules apply to captive-portal
submission in [provisioning-flow.md](provisioning-flow.md).

| Field | Rule | On violation |
|-------|------|--------------|
| Host | Non-empty; length 1–253 bytes | Reject with validation error |
| Port | Integer 1–65535; default 1883 when key missing | Reject with validation error |
| Username | Empty allowed (anonymous broker login) | — |
| Password | Empty allowed | — |
| Device ID | Empty allowed (MAC-derived at runtime); if non-empty, length 1–32 bytes, characters `[A-Za-z0-9_-]` only | Reject with validation error |
| TLS | Boolean; default false when key missing | Reject invalid value |

`mqtt_cred_is_stored` is true when NVDM `mqtt/host` exists and is non-empty.
A stored host is required for connect attempts (port defaults to 1883 when
`mqtt/port` is missing). Runtime `mqtt set` commands persist keys only — they
do not arm the client or start a connect; see session arming below.

## `mqtt` commands

Commands read and write NVDM group `mqtt` (`host`, `port`, `user`, `pass`,
`device_id`, `tls`). They do not start AP provisioning mode.

### `mqtt show`

Displays stored MQTT broker settings. Password is never printed in cleartext.

| Outcome | UART response |
|---------|---------------|
| Host unset | `host: (unset)` |
| Host set | `host: <value>` |
| Port unset | `port: 1883` (default) |
| Port set | `port: <value>` |
| User unset or empty | `user: (anonymous)` |
| User set | `user: <value>` |
| Password unset or empty | `pass: (empty)` |
| Password set | `pass: ********` |
| Device ID unset | `device_id: (mac)` |
| Device ID set | `device_id: <value>` |
| TLS unset | `tls: false` |
| TLS set | `tls: true` or `tls: false` |

### `mqtt set host <hostname>`

Validates and writes `mqtt/host` to NVDM. Does not connect.

| Outcome | UART response |
|---------|---------------|
| Success | `host saved` |
| Missing argument | `usage: mqtt set host <hostname>` |
| Validation failure | `invalid host` |
| NVDM write failure | `nvdm write failed` |

### `mqtt set port <port>`

Validates and writes `mqtt/port` to NVDM. Does not connect.

| Outcome | UART response |
|---------|---------------|
| Success | `port saved` |
| Missing argument | `usage: mqtt set port <port>` |
| Validation failure | `invalid port` |
| NVDM write failure | `nvdm write failed` |

### `mqtt set user <username>`

Writes `mqtt/user` to NVDM (empty string allowed).

| Outcome | UART response |
|---------|---------------|
| Success | `user saved` |
| Missing argument | `usage: mqtt set user <username>` |
| NVDM write failure | `nvdm write failed` |

### `mqtt set pass <password>`

Writes `mqtt/pass` to NVDM (empty string allowed).

| Outcome | UART response |
|---------|---------------|
| Success | `password saved` |
| Missing argument | `usage: mqtt set pass <password>` |
| NVDM write failure | `nvdm write failed` |

### `mqtt set device_id <id>`

Validates and writes `mqtt/device_id` to NVDM. Empty string clears the key
(MAC-derived ID used at runtime).

| Outcome | UART response |
|---------|---------------|
| Success | `device_id saved` |
| Missing argument | `usage: mqtt set device_id <id>` |
| Validation failure | `invalid device_id` |
| NVDM write failure | `nvdm write failed` |

### `mqtt set` (invalid subcommand)

| Outcome | UART response |
|---------|---------------|
| Missing or unknown subcommand | `usage: mqtt set host|port|user|pass|device_id <value>` |

### `mqtt connect`

Arms the client, loads broker settings from NVDM, validates, and starts MQTT
connect when Wi-Fi STA has a DHCP address. Runs in the MQTT client task (not
the CLI task).

| Precondition | Behavior |
|--------------|----------|
| `mqtt/host` not stored | Reject without connecting |
| Host stored | Queue connect; LWT on `.../state`; publish `{"online": true}` on success |

| Outcome | UART response |
|---------|---------------|
| Host not stored | `set host first` |
| Connect queued | `connecting...` |
| Connect already in progress | `connect already in progress` |
| Wi-Fi not ready | `wifi not ready` |

On successful broker connect, syslog prints (not necessarily inline with the
CLI prompt):

- `mqtt connecting to <host>:<port>` (once per connect attempt burst)
- `mqtt connected` (once per successful session)

While connected, the client does not log per-message or disconnect/reconnect
chatter on UART. LinkIt MQTT SDK debug (`[MQTT_CLIENT]: …`) is disabled in
the default build (`MTK_MQTT_DEBUG_ENABLE = n` in `feature.mk`) because the
SDK logs inside every `MQTTYield` loop and would flood the console.

| Failure | Log / behavior |
|---------|----------------|
| Missing or invalid NVDM settings | `no valid mqtt config in NVDM` (client task) |
| TCP or MQTT connect failure | `mqtt connect failed` (client task, once per attempt burst); exponential backoff reconnect while armed per [mqtt-protocol.md](mqtt-protocol.md) |
| TLS enabled in NVDM (`mqtt/tls` true) | `mqtt_cred_load` fails; connect does not proceed |

### `mqtt disconnect`

Stops the MQTT client: disconnects if connected, disarms the client, cancels
reconnect backoff, and does not retry until the next `mqtt connect` in the same
boot session. NVDM settings are unchanged.

| Outcome | UART response |
|---------|---------------|
| Success | `mqtt stopped` |

### Session arming

The client is **armed** when it may connect or reconnect automatically.
`mqtt connect` arms the client; exponential backoff applies while armed per
[mqtt-protocol.md](mqtt-protocol.md).

Runtime `mqtt set` commands write NVDM only — they do not arm the client or
start a connect. Use `mqtt connect` to arm and connect in the current boot
session, or reboot with `mqtt/host` already stored to auto-connect after Wi-Fi
DHCP.

`mqtt disconnect` disarms the client for the remainder of the boot session.
Reboot with a stored host arms and auto-connects again.

## Boot behavior (MQTT)

| Condition | Action |
|-----------|--------|
| Boot | Start `mqtt_io` task |
| `mqtt/host` stored in NVDM | Auto-connect once Wi-Fi STA has DHCP (no `mqtt connect` required) |
| No stored host | Remain disarmed until `mqtt connect` |
| After boot auto-connect or `mqtt connect` | Stay armed; reconnect with backoff if the session drops |
| `mqtt disconnect` (same boot session) | Disarm; no reconnect until next `mqtt connect` |

Subscription scope, online/LWT behavior, and command routing are defined in
[mqtt-protocol.md](mqtt-protocol.md#session-lifecycle).

## `display` commands

Bench helpers for runtime display exercise (WFCI bus loans after Wi-Fi init).
Logical commands use `display_presentation`; raw `fill` / `test` use
`display_port` directly. Not a product interface. `[design]`

On `display_port` or `display_presentation` failure, UART responses include the
`port_err_t` reason in parentheses, e.g. `display icon failed (busy)` or
`display number failed (invalid_arg)`. Reasons: `io`, `busy`, `invalid_arg`,
`not_found`, `not_supported`, `unknown`.

### `display test`

Runs the same segment pattern as boot self-test, but **after** `connsys_init()`
so each step uses the pin arbiter.

| Step | Action |
|------|--------|
| 1 | `power_on` |
| 2 | `show_fill(0xFF)` |
| 3 | Hold ~1 s (`DISPLAY_BOOT_LIGHT_TEST_MS`) |
| 4 | `blank` |
| 5 | `power_off` |

| Outcome | UART response |
|---------|---------------|
| Success | `display test ok` |
| Any `display_port` failure | `display test failed (<reason>)` |

### `display fill <hex_byte>`

Powers the rail, shows one segment byte on all digit grids, and leaves the
display on until `display off`.

| Argument | Rule |
|----------|------|
| `hex_byte` | One or two hex digits, value `0x00`–`0xFF` (case-insensitive) |

| Outcome | UART response |
|---------|---------------|
| Success | `display fill ok` |
| Missing argument | `usage: display fill <hex_byte>` |
| Invalid hex | `invalid hex byte` |
| `display_port` failure | `display fill failed (<reason>)` |

### `display off`

| Outcome | UART response |
|---------|---------------|
| Success | `display off ok` |
| `display_port` failure | `display off failed (<reason>)` |

### `display number <0-999> [g|%]`

Sets digits and optional unit via `display_presentation`. Powers on if needed.

| Argument | Rule |
|----------|------|
| value | Decimal 0–999 |
| unit | Optional: `g` or `%` (case-sensitive); omitted = no unit icon |

| Outcome | UART response |
|---------|---------------|
| Success | `display number ok` |
| Missing value | `usage: display number <0-999> [g|%]` |
| Invalid value | `invalid number` |
| Presentation failure | `display number failed (<reason>)` |

### Icon commands: steady state vs blink

Each pictograph has **two independent controls**:

| Control | Commands | What it does |
|---------|----------|--------------|
| **Steady state** | `on`, `off` | Resting visibility when the icon is not blinking. `on` = lit at rest; `off` = dark at rest. Persists until the next `on` or `off`. |
| **Blink** | `blink`, `steady` | Temporary square-wave override. `blink` toggles the icon on/off on a timer; `steady` **only** cancels that toggle. |

`steady` is **not** a synonym for `on`. It does not force the icon lit — it ends
blinking and returns to whatever steady state was already set.

Example — Wi‑Fi associating indicator (production uses `[tune]` 500/500 ms;
see [display-presentation.md](display-presentation.md) § Wi-Fi indicator):

```text
display icon wifi on              # resting state: Wi‑Fi lit when idle
display icon wifi blink 500 500   # while connecting: flash on that schedule
display icon wifi steady          # done connecting: stop flashing, stay lit (on)
```

AP provisioning blink on the device uses `[tune]` 150/150 ms (not exposed as a
separate UART preset).

If steady state was `off` before `blink`, `steady` leaves the icon **off** even
if the blink happened to be in its visible phase when you stopped it.

### `display icon <name> on|off`

Sets **steady state** only. Does not start or stop blink.

| Argument | Rule |
|----------|------|
| name | One of: `child_lock`, `wifi`, `dispensing`, `percent`, `gram`, `blockage`, `insufficient_food`, `bowl_error`, `bar_orange`, `bar_green` |
| state | `on` or `off` |

| Outcome | UART response |
|---------|---------------|
| Success | `display icon ok` |
| Missing args | `usage: display icon <name> on|off` |
| Unknown name | `unknown icon` |
| Presentation failure | `display icon failed (<reason>)` |

### `display icon <name> blink <on_ms> <off_ms>`

Starts square-wave blink on the named icon. Overrides steady visibility while
active. Each duration 50–5000 ms. Steady state (`on`/`off`) is remembered and
restored by `steady`.

| Outcome | UART response |
|---------|---------------|
| Success | `display icon blink ok` |
| Missing args | `usage: display icon <name> blink <on_ms> <off_ms>` |
| Unknown name | `unknown icon` |
| Invalid timing | `invalid blink timing` |
| Presentation failure | `display icon blink failed (<reason>)` |

### `display icon <name> steady`

Stops blink on the named icon. **Does not change steady state** — icon visibility
after `steady` is whatever `on` or `off` was set to before (or since) the blink
started.

| Outcome | UART response |
|---------|---------------|
| Success | `display icon steady ok` |
| Unknown name | `unknown icon` |
| Presentation failure | `display icon steady failed (<reason>)` |

### `display anim <ota|lock> [loop]`

Plays built-in animation. Optional `loop` repeats until `display anim stop`.

| Outcome | UART response |
|---------|---------------|
| Success | `display anim ok` |
| Missing name | `usage: display anim <ota|lock> [loop]` |
| Unknown name | `unknown animation` |
| Presentation failure | `display anim failed (<reason>)` |

### `display anim stop`

Stops animation and restores steady scene.

| Outcome | UART response |
|---------|---------------|
| Success | `display anim stop ok` |

### `display brightness <1-4>`

| Outcome | UART response |
|---------|---------------|
| Success | `display brightness ok` |
| Missing level | `usage: display brightness <1-4>` |
| Invalid level | `invalid brightness` |
| Presentation failure | `display brightness failed (<reason>)` |

## `weigh` commands

Bench helpers for CS1270 load-cell exercise (WFCI `WEIGH` bus loan after
Wi-Fi init). Uses `weight_port`. Not a product interface. `[design]`

`weigh read` returns **absolute** food grams now (stateless weigh driver
except NVDM cal). There is no `weigh tare` — deltas belong in dispense/monitoring
([weighing.md](weighing.md) **Weigh driver boundary**).

Scale-off and missing-calibration cases use explicit messages (see below).
Other `weight_port` failures include the `port_err_t` reason in parentheses,
e.g. `weigh read failed (io)`.

### `weigh power on`

Powers CS1270 via AW9523B P0.2, releases the expander loan, then waits boot
settle (`[tune]` 1100 ms with WFCI restored).

| Outcome | UART response |
|---------|---------------|
| Success | `weigh power on ok` |
| Failure | `weigh power on failed (<reason>)` |

### `weigh power off`

| Outcome | UART response |
|---------|---------------|
| Success | `weigh power off ok` |
| Failure | `weigh power off failed (<reason>)` |

### `weigh read`

Queries food grams (empty installed bowl = 0 g). Auto power-on on first use.
After `weigh power off`, does **not** re-power the rail — operator must run
`weigh power on` first.

| Outcome | UART response |
|---------|---------------|
| Success | `weight: <signed grams> g` |
| Rail off (after `weigh power off`) | `weigh read: scale off (weigh power on first)` |
| No calibration | `weigh read: no calibration (weigh cal zero, then weigh cal span)` then, if the chip returns a weight frame, `weight: <n> g (raw, no calibration)` |
| Zero only (span pending) | `weigh read: calibration incomplete (install bowl, weigh cal span)` then optional raw line as above |
| Failure | `weigh read failed (<reason>)` |

### `weigh cal zero`

Capture raw count with bowl **removed**; save `calib/zero` to NVDM. Clears
span coefficients.

| Outcome | UART response |
|---------|---------------|
| Success | `weigh cal zero ok` then `install provided bowl, then: weigh cal span` |
| Failure | `weigh cal zero failed (<reason>)` |

### `weigh cal span`

Capture raw count with provided bowl installed (350 g reference); save
`calib/span_g` + `calib/span_raw`. Requires prior `weigh cal zero`.

| Outcome | UART response |
|---------|---------------|
| Success | `weigh cal span ok` |
| Zero not captured / chip not ready | `weigh cal span failed (not_supported)` or `invalid_arg` |
| Failure | `weigh cal span failed (<reason>)` |

### `weigh cal status`

| Outcome | UART response |
|---------|---------------|
| Success | `weigh cal: idle`, `capturing_span`, `success`, or `uncalibrated` |

## `config` commands

Bench helper for factory reset without the pin-hole button. Same effect as
long press in [provisioning-flow.md](provisioning-flow.md#re-provisioning-pin-hole-button-p04).

### `config factory-reset`

Erases all application NVDM namespaces (`wifi`, `mqtt`, `feed`, `display`,
`schedule`, `time`, `calib`, `power`, `system`) and reboots immediately.
Erasing a namespace that has no keys (never written) is success — the goal is
an empty store, not that every group must have existed beforehand.

| Outcome | UART response |
|---------|---------------|
| Success | `factory reset — rebooting` then reboot |
| NVDM erase failure | `factory reset failed` |
