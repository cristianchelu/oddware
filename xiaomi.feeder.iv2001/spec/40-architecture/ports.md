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

Adapter: wraps SDK `httpd` middleware. The SDK `httpd_proc` task stack is
sized for provisioning HTML generation (see `HTTPD_TASK_STACKSIZE` in
`task_def.h`). Used for the provisioning captive
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

### `weight_port.h`

**Weigh driver boundary** ([weighing.md](../30-processes/weighing.md)): the
driver is stateless except NVDM cal. `read_grams` returns absolute food in the
bowl now. Deltas (`eaten_today`, dispense delivered grams) are computed by
dispense/monitoring tasks that snapshot `read_grams` before and after events —
not by offsets inside this port.

| Function | Signature | Behavior |
|----------|-----------|----------|
| `boot_begin` | `() -> err` | EXPANDER loan → P0.2 high → release; arm non-blocking 1100 ms settle (`app` boot FSM) |
| `boot_poll` | `() -> err` | `PORT_ERR_BUSY` until settle elapses; then `boot_done` |
| `power_on` | `() -> err` | `boot_begin` + blocking `boot_poll` loop (CLI / `weigh power on`) |
| `power_off` | `() -> err` | Disable CS1270 |
| `read_grams` | `(int32_t *grams) -> err` | Absolute food grams now (requires host cal; empty bowl = 0) |
| `try_read_grams` | `(int32_t *grams) -> err` | Same as `read_grams` but `try_acquire` on WFCI `WEIGH` — `PORT_ERR_BUSY` when Wi-Fi holds the bus (idle loop; CLI uses blocking `read_grams`) |
| `read_raw_grams` | `(int32_t *grams) -> err` | Uncorrected CS1270 count (bench) |
| `calibrate_zero` | `() -> err` | Capture raw with bowl removed → NVDM `calib/zero` |
| `calibrate_span` | `() -> err` | Capture raw with bowl installed → NVDM `calib/span_*` (350 g) |
| `get_cal_status` | `() -> weight_cal_status_t` | Host cal state from NVDM |

Adapter: wraps UART2 serial protocol to CS1270 via WFCI `WEIGH` bus loan.
See [weigh-assp-cs1270.md](../10-hardware/components/weigh-assp-cs1270.md).

### `wfci_bus_port.h`

Central bus-loan layer below peripheral ports and above WFCI HAL. Time-multiplexes
GPIO12–17 between the Wi-Fi N9 SPI link and feeder peripherals while keeping
association up. See [wfci-bus-arbitration.md](../30-processes/wfci-bus-arbitration.md).

| Function | Signature | Behavior |
|----------|-----------|----------|
| `acquire` | `(profile, priority, timeout_ms) -> err` | Block until WFCI SPI is idle; `wfcm_if_deinit()`; remux pins per profile; init HAL peripheral (I2C1 / UART2 / GPIO / AUXADC) |
| `try_acquire` | `(profile, priority) -> err` | Non-blocking variant; returns `PORT_ERR_BUSY` when loan unavailable |
| `release` | `(profile) -> void` | Deinit peripheral HAL; `wfcm_if_reinit()`; release SDK bus mutex |

**Profiles:** `EXPANDER`, `DISPLAY`, `ADC`, `WEIGH`, `FULL` — encode board
knowledge from [pinmap.md](../10-hardware/pinmap.md); application code does not
use raw GPIO numbers.

**Priorities:** `NORMAL`, `ABOVE_NORMAL`, `HIGH`.

Adapter: `wfci_bus_adapter.c` coordinates with `wfcm_bus_loan.c`
(`wfcm_bus_loan_begin()` / `wfcm_bus_loan_end()`). `wfci_bus_wifi_spi_active(true)`
is called after `connsys_init()`; before that, acquire/release are no-ops.

### Display stack ports

`[design]` Three ports separate infrastructure, driver, and presentation:

- **`wfci_bus_port`** — arbitration for contested GPIO12–17 (see above).
- **`i2c_bus_port`** + **`gpio_expander_port`** — infrastructure (AW9523B @
  `0x58`). Used by display rail and future motor/weight subsystems. **Not**
  called from presentation code. After Wi-Fi init, every I2C transaction runs
  inside an `EXPANDER` loan or nested micro-session.
- **`display_port`** — presentation-facing rendering API.

Physical seam: AW9523B **P0.5** switches display power (I2C); TM1637 segment
data uses SoC **GPIO1/GPIO13** only. Only `display_driver.c` sequences
rail-on before TM1637 traffic. See [display-driver.md](../30-processes/display-driver.md)
§ Software layering.

### `i2c_bus_port.h`

| Function | Signature | Behavior |
|----------|-----------|----------|
| `write_reg` | `(addr, reg, val) -> err` | I2C write one register byte |
| `read_reg` | `(addr, reg, &val) -> err` | I2C read one register byte |

Adapter: wraps HAL I2C master (I2C1: GPIO15 = SCL, GPIO16 = SDA per
[pinmap.md](../10-hardware/pinmap.md)). Init/deinit is driven by
`wfci_bus_port` when Wi-Fi SPI is active; boot-time access before
`connsys_init()` calls `i2c_bus_adapter_init()` directly.

### `button_port.h`

**Process boundary** for user-facing tactile buttons ([button-handling.md](../30-processes/button-handling.md)).
`[design]` The AW9523B expander is shared infrastructure (display rail, motor
lines, hopper IR, tactiles). Application and debounce logic depend on
`button_port`, not `gpio_expander_port`. Today’s adapter reads expander input
registers and maps active-low pins to `button_sample_t`; hopper broken-beam IR
will use a separate adapter on the same GPIO4 IRQ line.

| Function | Signature | Behavior |
|----------|-----------|----------|
| `read_sample` | `(&sample) -> err` | Fill `power_pressed`, `reset_pressed`, `dispense_pressed` (`true` = down) |

Adapter: `button_port_adapter.c` — `gpio_expander_port.read_inputs` +
`board_gpio_iv2001.h` masks. HAL IRQ wiring: `button_adapter.c` (GPIO4 EINT).

### `gpio_expander_port.h`

| Function | Signature | Behavior |
|----------|-----------|----------|
| `reset` | `() -> err` | Hardware reset pulse on GPIO14, verify ID `0x23` |
| `configure` | `(dir_p0, dir_p1, out_p0, out_p1) -> err` | Write direction and output registers |
| `set_pin` | `(port, pin, level) -> err` | Set one expander pin (0=output, 1=input per AW9523B) |
| `get_pin` | `(port, pin, &level) -> err` | Read one expander input pin |
| `read_inputs` | `(&p0, &p1) -> err` | Read input registers 0x00 and 0x01 in one `EXPANDER` loan |
| `set_int_mask` | `(mask_p0, mask_p1) -> err` | Write IRQ mask registers 0x06/0x07 (`0` = enabled, `1` = masked) |

Adapter: `gpio_expander_adapter.c` — AW9523B register model + `i2c_bus_port`.
Bootstrap bitmaps: `board_gpio_iv2001.h`. Micro-session helpers
`gpio_expander_loan_begin()` / `gpio_expander_loan_end()` acquire `EXPANDER`
for batched register writes; never held across motor spin or wait loops.

### `display_port.h`

Bootstrap subset implemented; full API grows with display features.

| Function | Signature | Behavior |
|----------|-----------|----------|
| `power_on` | `() -> err` | Expander bootstrap + rail on + 100 ms settle + TM1637 init |
| `power_off` | `() -> err` | P0.5 low via expander |
| `show_fill` | `(segment_byte) -> err` | Requires rail settled; grids 0–4 same byte, grid 5 `0x00`; uses stored brightness |
| `show_grids` | `(grids[5]) -> err` | Five payload bytes (grids 0–4); blocking WFCI `DISPLAY` loan (CLI) |
| `try_show_grids` | `(grids[5]) -> err` | Same as `show_grids` but `try_acquire` — skip frame on `PORT_ERR_BUSY` (presentation tick) |
| `blank` | `() -> err` | `show_fill(0x00)` |
| `set_brightness` | `(level) -> err` | Brightness 1–4 → `0x88`–`0x8B`; persists until next call |

Logical composition (digits, units, icon labels) lives in `display_glyph.c` and
`display_presentation.c` — not on `display_port`. See
[display-presentation.md](../30-processes/display-presentation.md).

Adapter: `display_adapter.c` binds `display_driver.c` to HAL GPIO and
`gpio_expander_port`. Full refresh takes ~1 ms. See
[display-tm1637.md](../10-hardware/components/display-tm1637.md).
