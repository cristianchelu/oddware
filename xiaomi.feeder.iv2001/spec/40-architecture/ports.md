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

See [wifi-lifecycle.md](../30-processes/wifi-lifecycle.md) for connect,
disconnect, and wait semantics.

| Function | Signature | Behavior |
|----------|-----------|----------|
| `disconnect` | `() -> err` | `lwip_net_stop(STA)` → `disconnect_ap` → `set_radio(0)`; idempotent |
| `radio_up` | `() -> err` | `set_radio(1)` and `lwip_net_start(STA)` when stopped |
| `connect` | `(ssid, pass) -> err` | Set SSID/PSK (or open), `reload_setting()`; **does not block** for DHCP |
| `wait_ready` | `(timeout_ms) -> err` | Block on SDK `PORT_SECURE` + DHCP semaphores via `lwip_net_ready_timed`; fails fast if stack init not complete |
| `is_connected` | `() -> bool` | Current association state |
| `get_ip` | `(buf, len) -> err` | Copy current IP string into buffer |
| `start_ap` | `(ssid, pass, channel) -> err` | Start AP mode for provisioning |
| `stop_ap` | `() -> err` | Tear down AP mode |

Adapter: wraps SDK `wifi_init()`, `wifi_connection_register_event_handler()`,
`lwip_network_init()`, and `wifi_lwip_helper` semaphores. Session orchestration
(`radio_up` → `connect` → `wait_ready`) lives in `wifi_session.c`; the adapter
binds individual SDK calls only.

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

### `motor_port.h`

**Auger H-bridge** ([dispense-cycle.md](../30-processes/dispense-cycle.md) PH/EN
sequencing). All CLI and product motor motion enqueues to `motor_ctrl`; the
adapter does not block the CLI task on PH settle + spin.

| Function | Signature | Blocking? | Behavior |
|----------|-----------|-----------|----------|
| `request_timed_forward_ms` | `(duration_ms) -> err` | **No** | Enqueue bench forward timed run to `motor_ctrl`. PH forward, `[tune]` 100 ms settle, EN run for `duration_ms`, auto coast-stop. No index or ADC jam supervision. Posts `EVT_TIMED_RUN_DONE` or `EVT_MOTOR_FAULT`. `PORT_ERR_BUSY` if queue full or motor active. `PORT_ERR_INVALID_ARG` if out of range (`1…20000` ms). |
| `request_timed_reverse_ms` | `(duration_ms) -> err` | **No** | Enqueue bench reverse timed run; same semantics as `request_timed_forward_ms` with PH reverse. |
| `request_burst` | `(pulse_target, timeout_ms) -> err` | **No** | Enqueue burst to `motor_ctrl` command queue with send timeout 0. Index LED on, forward EN until `pulse_target` beam-open edges or `timeout_ms` (whichever first). Posts `EVT_BURST_DONE` or `EVT_MOTOR_FAULT`. `PORT_ERR_BUSY` if queue full or motor active. |
| `request_park` | `(max_pulses) -> err` | **No** | Enqueue park to `motor_ctrl` (CLI recovery only in v1). Forward until beam-open or `max_pulses`. Posts `EVT_PARK_DONE` or `EVT_MOTOR_FAULT`. |
| `stop` | `() -> err` | **No** | Enqueue preemptive stop to `motor_ctrl` (timeout 0). |
| `is_active` | `() -> bool` | — | True while `motor_ctrl` owns a burst, park, bench timed run, or anti-jam sequence. |

Adapter: `motor_adapter.c` — all entries enqueue to `motor_ctrl` only.
`motor_ctrl` holds the adapter mutex for the full burst/park/bench/anti-jam
sequence and calls `motor_driver_start_*` / `motor_driver_stop` directly.

Results arrive as `EVT_BURST_DONE`, `EVT_TIMED_RUN_DONE`, `EVT_MOTOR_FAULT`, or
`EVT_PARK_DONE` in `app_event_q` (`app_event_post` timeout 0).

### `adc_port.h`

**BAT/MOT analog sense** ([battery-monitoring.md](../30-processes/battery-monitoring.md),
[jam-detection.md](../30-processes/jam-detection.md), [analog-mux-nc7sb3157.md](../10-hardware/components/analog-mux-nc7sb3157.md)).
NC7SB3157 select on AW9523B P1.7; sample on MT7682 GPIO17 (AUXADC0) via
`WFCI_BUS_PROFILE_ADC` micro-loans. Bench UART `adc read motor|battery` call
this port synchronously on the CLI task.

| Function | Signature | Behavior |
|----------|-----------|----------|
| `read_motor_load_ma` | `(&ma) -> err` | Idempotent P1.7 low (motor path B0→COM), `[tune]` 1 ms mux settle, **1** raw sample, `ma = raw × 2500 / 4095`. Value is motor current in mA (1 Ω shunt — `[probe]`; no scale). `PORT_ERR_BUSY` if adapter mutex not acquired within `[tune]` 5000 ms or WFCI `ADC` loan unavailable. |
| `read_battery_mv` | `(&mv) -> err` | If motor EN (P0.1) high → `PORT_ERR_BUSY`. P1.7 high (battery path), settle, **10** raw samples with outlier trim, average pin mV, apply `pin_mV × batt_scale_x1000 / 1000` from NVDM (default 11000). **Always** restore P1.7 low. Same mutex / loan errors as motor load. |
| `cal_capture` | `(true_mv) -> err` | One-point divider trim: sample pin mV on battery path (same exclusivity as `read_battery_mv`), store `round(true_mv × 1000 / pin_mV)` as `power/batt_scale_x1000`, update runtime cal. `true_mv` `[design]` 3000–8000. |
| `cal_reset` | `() -> err` | Erase `power/batt_scale_x1000`; restore 11.000 default. |
| `get_cal_status` | `(&status) -> err` | Fill `adc_cal_status_t` (`scale_x1000`, `customized`) from driver state / NVDM. |

Adapter: `adc_adapter.c` — `gpio_expander_port` + `adc_driver.c` + `adc_bus_adapter.c`
(HAL AUXADC0); mutex for overlapping callers.

| `try_read_motor_load_ma` | `(&ma) -> err` | **No** | Same sample path as `read_motor_load_ma` but mutex/WFCI acquire with timeout 0 — `PORT_ERR_BUSY` when contended. Used by `motor_ctrl` jam loop only. |

**Not exposed this slice** (future monitoring):

| Function | When |
|----------|------|
| `try_read_battery_mv` | Monitoring tick when WFCI busy |

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
registers and maps pin polarity to `button_sample_t` (P0.3/P0.4 active-low,
P1.0 active-high per `[probe]`); hopper broken-beam IR
will use a separate adapter on the same GPIO4 IRQ line.

| Function | Signature | Behavior |
|----------|-----------|----------|
| `read_sample` | `(&sample) -> err` | Fill `power_pressed`, `reset_pressed`, `dispense_pressed` (`true` = down) |

Adapter: `button_port_adapter.c` — `gpio_expander_port.read_inputs` +
`board_gpio_iv2001.h` masks. HAL IRQ wiring: `aw9523_irq_adapter.c` (GPIO4 EINT);
`button_adapter.h` forwards to `aw9523_irq_adapter_start()`.

### `motor_index_port.h`

**Motor index broken-beam** ([motor-index.md](../30-processes/motor-index.md)).
AW9523B P0.6 (LED) and P0.7 (detector). Application and `motor_ctrl` depend on
`motor_index_port`, not `gpio_expander_port`. LED drive and P0.7 sampling stay
inside the adapter — callers never toggle P0.6 or read the expander directly.

| Function | Signature | Behavior |
|----------|-----------|----------|
| `sense` | `(&beam_open) -> err` | One-shot: illuminate → sample → de-illuminate; `true` = hole aligned / beam open |
| `session_begin` | `() -> err` | Illuminate for an active motor burst/park session (idempotent) |
| `session_end` | `() -> err` | De-illuminate after session teardown |
| `poll` | `(&beam_open) -> err` | Sample while `session_begin` is active; `PORT_ERR_IO` if session inactive |

Adapter: `motor_index_port_adapter.c` — `gpio_expander_port` +
`board_gpio_iv2001.h` masks. Polarity: `BOARD_GPIO_INDEX_BEAM_OPEN_HIGH`.

IRQ bootstrap (outside port struct): `motor_index_adapter_arm_irq(task, bits)` /
`motor_index_adapter_disarm_irq()` register a `xTaskNotifyFromISR` target on the
shared GPIO4 AW9523B INT line via `aw9523_irq_adapter`. Used by `motor_ctrl` for
low-latency pulse edges; no I2C in the ISR.

### `hopper_ir_port.h`

**Hopper low-fill broken-beam** ([hopper-sensing.md](../30-processes/hopper-sensing.md)).
MT7682 GPIO0 (IR drive) and AW9523B P1.4 (sense). Polled only in this phase — no
IRQ path on the process port.

| Function | Signature | Behavior |
|----------|-----------|----------|
| `sense` | `(&beam_blocked) -> err` | Pulse GPIO0 for `[tune]` ~1 ms, read P1.4; `true` = food blocks beam |

Adapter: `hopper_ir_port_adapter.c` — HAL GPIO0 + `gpio_expander_port.read_inputs`.
Polarity: `BOARD_GPIO_HOPPER_BEAM_BLOCKED_HIGH`. GPIO0 is outside WFCI — no bus
loan for the drive pin.

### `gpio_expander_port.h`

| Function | Signature | Behavior |
|----------|-----------|----------|
| `reset` | `() -> err` | Hardware reset pulse on GPIO14, verify ID `0x23` |
| `configure` | `(dir_p0, dir_p1, out_p0, out_p1) -> err` | Write direction and output registers |
| `set_pin` | `(port, pin, level) -> err` | Set one expander pin (0=output, 1=input per AW9523B) |
| `get_pin` | `(port, pin, &level) -> err` | For pins configured as **output** (`dir` bit = 0), return the commanded level from the output latch. For **input** pins, read the input register. Motor EN exclusivity uses output latch, not pad sense (FAULT shares EN — `[probe]`). |
| `read_inputs` | `(&p0, &p1) -> err` | Read input registers 0x00 and 0x01 in one blocking `EXPANDER` loan |
| `try_read_inputs` | `(&p0, &p1) -> err` | Same; `try_acquire` — `PORT_ERR_BUSY` when WFCI contended (motor index `poll`) |
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
