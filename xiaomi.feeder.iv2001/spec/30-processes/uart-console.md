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
