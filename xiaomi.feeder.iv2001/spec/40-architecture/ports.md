# Ports and adapters

## Hexagonal architecture

The firmware follows a hexagonal (ports and adapters) pattern. Application
logic depends only on port interfaces, never on SDK headers directly.
`[design]`

```
                +------------------------------+
                |          Application          |
                |  (OTA, provision, dispense,   |
                |   schedule, weighing, ...)    |
                +------------------------------+
                      |                  |
                [Port: WiFi]       [Port: MQTT]
                [Port: Flash]      [Port: Config]
                [Port: HTTP]       [Port: Timer]
                [Port: Motor]      [Port: Weight]
                [Port: Display]
                      |                  |
                +------------------------------+
                |     Adapters (SDK bindings)   |
                +------------------------------+
                      |
                +------------------------------+
                |     LinkIt SDK / MT7682 HAL   |
                +------------------------------+
```

### Ports

C interfaces defined as header-only structs of function pointers. Pure
contracts with no SDK dependency. Each port header lives in
`firmware/ports/`.

### Adapters

Implementations of ports against the real SDK HAL and middleware. Each
adapter lives in `firmware/adapters/` and includes the necessary SDK
headers.

### Test doubles

Fake implementations of ports for host-side testing. Each fake lives in
`firmware/test/fakes/`. Fakes record calls and return configurable values,
enabling assertion-based testing without hardware.

## Port contracts

### `wifi_port.h`

| Function | Signature | Behavior |
|----------|-----------|----------|
| `connect` | `(ssid, pass) -> err` | Start STA association, return immediately; result arrives as `EVT_WIFI_CONNECTED` or `EVT_WIFI_DISCONNECTED` |
| `is_connected` | `() -> bool` | Current association state |
| `get_ip` | `(buf, len) -> err` | Copy current IP string into buffer |
| `start_ap` | `(ssid, pass, channel) -> err` | Start AP mode for provisioning |
| `stop_ap` | `() -> err` | Tear down AP mode |

Adapter: wraps SDK `wifi_init()`, `wifi_connection_register_event_handler()`,
`lwip_network_init()`.

### `mqtt_port.h`

| Function | Signature | Behavior |
|----------|-----------|----------|
| `connect` | `(cfg) -> err` | Connect to broker with LWT, credentials from config |
| `disconnect` | `() -> err` | Clean disconnect |
| `publish` | `(topic, payload, qos, retain) -> err` | Publish message |
| `subscribe` | `(topic, qos) -> err` | Subscribe; messages arrive as `EVT_MQTT_MESSAGE` |
| `set_lwt` | `(topic, payload, qos, retain) -> err` | Configure Last Will at connect time |

Adapter: wraps SDK `middleware/third_party/mqtt/MQTTClient-C`. The
`mqtt_io` task runs `MQTTYield()` and posts received messages to
`app_event_q`.

### `flash_bank_port.h`

| Function | Signature | Behavior |
|----------|-----------|----------|
| `get_active_bank` | `() -> boot_bank_t` | Returns `BOOT_BANK_A` or `BOOT_BANK_B` based on control block |
| `erase_inactive` | `() -> err` | Erase the inactive bank |
| `write_inactive` | `(offset, data, len) -> err` | Write to inactive bank at offset |
| `verify_inactive` | `(expected_hash[64], image_len) -> err` | Compute SHA-512 over inactive bank image, compare |
| `swap_banks` | `(image_hash[64]) -> err` | Flip active flag in control block; next boot targets other bank |

Adapter: wraps the adapted `fota_dual_image` APIs and `hal_flash_*` calls.
See [sdk-reference.md](sdk-reference.md) for the FOTA dual-image
adaptation.

### `ota_port.h`

| Function | Signature | Behavior |
|----------|-----------|----------|
| `start` | `(url, expected_sha512, has_expected_sha512) -> err` | Begin OTA download to inactive bank; spawns `ota_dl` task. `expected_sha512` is 64 bytes; when `has_expected_sha512` is false, verify uses the hash computed from the downloaded image |
| `get_status` | `() -> ota_status_t` | Current OTA state: `idle`, `downloading`, `verifying`, `applying`, `error` |
| `abort` | `() -> err` | Cancel in-progress download |
| `set_progress_cb` | `(cb, ctx) -> void` | Register callback invoked on status/progress updates (`ota_progress_t`: `status`, `pct`, `error`) |

Adapter: wraps SDK `httpclient` middleware for download, delegates flash
operations to `flash_bank_port`.

### `http_server_port.h`

| Function | Signature | Behavior |
|----------|-----------|----------|
| `start` | `(port) -> err` | Start HTTP server on given port |
| `stop` | `() -> err` | Stop HTTP server |
| `register_route` | `(method, path, handler) -> err` | Register a route handler |

Adapter: wraps SDK `httpd` middleware. Used for the provisioning captive
portal.

### `config_port.h`

| Function | Signature | Behavior |
|----------|-----------|----------|
| `read` | `(group, key, buf, len) -> err` | Read a string value from persistent storage |
| `write` | `(group, key, value) -> err` | Write a string value to persistent storage |
| `erase` | `(group, key) -> err` | Delete a key |
| `erase_group` | `(group) -> err` | Delete all keys in a group (factory reset) |

Adapter: wraps SDK NVDM API. Groups map to NVDM groups (e.g. `wifi`,
`mqtt`, `schedule`).

### `motor_port.h` (future, with dispense features)

| Function | Signature | Behavior |
|----------|-----------|----------|
| `start_burst` | `(direction, duration_ms, max_pulses) -> err` | Command to `motor_ctrl` task via queue |
| `stop` | `() -> err` | Immediate motor stop |
| `park` | `() -> err` | Run to seal index position |

Adapter: sends commands to `motor_ctrl` task. Results arrive as
`EVT_BURST_DONE`, `EVT_MOTOR_FAULT`, or `EVT_PARK_DONE` in `app_event_q`.

### `weight_port.h` (future, with dispense features)

| Function | Signature | Behavior |
|----------|-----------|----------|
| `power_on` | `() -> err` | Enable CS1270 via AW9523B P0.2 |
| `power_off` | `() -> err` | Disable CS1270 |
| `read_grams` | `() -> int32` | Blocking UART2 command/response, returns weight in grams |
| `tare` | `() -> err` | Zero-point calibration |
| `calibrate_span` | `(known_grams) -> err` | Span calibration with known weight |

Adapter: wraps UART2 serial protocol to CS1270.

### `display_port.h` (future, with display features)

| Function | Signature | Behavior |
|----------|-----------|----------|
| `power_on` | `() -> err` | Enable display rail via AW9523B P0.5, wait settle |
| `power_off` | `() -> err` | Disable display rail |
| `show_number` | `(value, unit) -> err` | Render 0–999 on digits with unit icon |
| `show_icons` | `(icon_mask) -> err` | Set/clear pictograph and status bar bits |
| `set_brightness` | `(level) -> err` | Brightness 1–4 |

Adapter: wraps TM1637 GPIO bit-bang protocol on GPIO1/GPIO13. Full refresh
takes ~1 ms in a brief critical section for timing. See
[display-tm1637.md](../10-hardware/components/display-tm1637.md).
