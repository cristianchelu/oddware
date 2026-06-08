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

Displays stored Wi-Fi credentials. Password is never printed in cleartext.

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
| No stored SSID | Remain idle until `wifi connect` or future provisioning |

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
