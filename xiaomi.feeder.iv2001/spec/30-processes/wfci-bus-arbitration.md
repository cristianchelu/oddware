# WFCI bus arbitration

`serves:` feeding, monitoring, power management, display presentation

The MT7682 shares GPIO12–17 between the WFCI link to the Wi-Fi N9 coprocessor
and feeder peripherals (AW9523B I2C, TM1637 CLK, UART2 RX, AUXADC). Wi-Fi
association must stay up during motor, display, weighing, and button work.
`[design]`

## Contested pins

| WFCI SPI role | GPIO | Feeder use |
|---------------|------|------------|
| SIO_1 (MISO) | 14 | AW9523B RESET |
| SIO_0 (MOSI) | 15 | I2C1 SCL |
| CLK | 16 | I2C1 SDA |
| CS | 17 | NC7SB3157 COM (battery / motor ADC) |
| SIO_2 | 13 | TM1637 CLK |
| SIO_3 | 12 | CS1270 UART2 TX (UTXD2) |

GPIO1 (TM1637 DIO), GPIO4 (AW9523B INT), GPIO11 (UART2 RX / URXD2), and GPIO0
(hopper IR) are outside the WFCI block and need no loan.

Cold boot runs `display_boot_run()` before `connsys_init()` — pins are still
application-owned; no arbiter. See [build-integration.md](../40-architecture/build-integration.md)
§ Display boot before Wi-Fi SPI.

After `connsys_init()`, every access to contested pins goes through
`wfci_bus_port` (Tier 4: [ports.md](../40-architecture/ports.md)).

## Bus loan model

An **association-preserving bus loan** stalls WFCI SPI, calls
`wfcm_if_deinit()`, remuxes pins for a bounded window, runs peripheral HAL
work, then `wfcm_if_reinit()` and releases the loan. Wi-Fi is not torn down;
SPI idles briefly while expander outputs latch.

**Micro-loans only.** Never hold a loan across motor spin (2–5 s per burst),
reverse delay, dispense completion, or display refresh timer period.
`[tune]` Max single loan: 5 ms for `EXPANDER`, `DISPLAY`, `ADC`; 50 ms for
`WEIGH`; 200 ms for `FULL` (sleep/wake).

AW9523B outputs latch in silicon; motor EN can stay asserted while WFCI is
free. Motor control uses **try_acquire** expander loans for index `poll` during
motion (skip sample on `PORT_ERR_BUSY` — no I2C attempt). Blocking acquire
remains for park pre-check `sense` and session `set_pin` edges. Index IRQ
samples are debounced `[tune]` 50 ms before one I2C read (retried on
`PORT_ERR_BUSY`). Session fallback polls: burst `[tune]` 50 ms, park `[tune]`
200 ms.

## Bus profiles

| Profile | Pins / HAL | Typical consumer |
|---------|------------|------------------|
| `EXPANDER` | GPIO14–16, I2C1 | AW9523B reset, configure, read/write |
| `DISPLAY` | `EXPANDER` + GPIO13 TM1637 CLK | `display_port` refresh (~1 ms) |
| `ADC` | GPIO17 AUXADC | Jam detect, battery |
| `WEIGH` | `EXPANDER` + GPIO12 UART2 TX | CS1270 sample window |
| `FULL` | All contested pins | Sleep entry / wake |

Application code uses profiles only — not raw GPIO numbers.

## Priority

| Consumer | Priority | Hold pattern |
|----------|----------|--------------|
| Motor burst edge, jam stop, index IRQ | HIGH | Micro-loan `EXPANDER` / `ADC` |
| Button debounce / mains IRQ | ABOVE_NORMAL | Scoped `EXPANDER` |
| Weight idle sample | NORMAL | `WEIGH` ~50 ms; `try_acquire` — skip tick on `PORT_ERR_BUSY` |
| Display presentation | NORMAL | `DISPLAY` < 5 ms |
| Sleep entry / wake | NORMAL | `FULL` or `EXPANDER` ~200 ms |

`[design]` Blocking `acquire` serializes all profiles through a binary loan
semaphore: if another micro-loan is in flight, the caller blocks in the kernel
until `release` or `timeout_ms` elapses (whichever comes first). When Wi-Fi SPI
is active, the loan wait and the Wi-Fi arbiter wait share one `timeout_ms`
budget (remaining time after the loan semaphore is granted). `try_acquire`
remains fail-fast (`timeout_ms` 0) — display may skip a frame; idle weight
keeps the last good sample on `PORT_ERR_BUSY`.

## Coexistence with MQTT TCP connect

After `connsys_init()`, the Wi-Fi N9 link holds WFCI SPI for long stretches
during TCP and MQTT handshakes. The feeder does **not** block `app` or
`mqtt_io` on those calls — the handshake runs on `mqtt_cn` at priority below
`app` (see [mqtt-protocol.md](mqtt-protocol.md) § Connect execution).

Idle weight and TM1637 refresh use `try_acquire` on `WEIGH` and `DISPLAY`
respectively. A failed try leaves the last good gram sample in presentation
scene (`PORT_ERR_BUSY`) or retries the frame on the next `[tune]` 50 ms
display tick. Blocking `acquire` with multi-second timeout on the presentation
path during connect starves the panel even when `app` runs.

UART `weigh` / `display` CLI commands use blocking port functions — acceptable
for bench use only.

## GPIO expander micro-session

Batch AW9523B writes in one loan, then release before waiting:

```
gpio_expander_loan_begin()   → acquire EXPANDER
  aw9523b_set_output / flush
gpio_expander_loan_end()     → release; WFCI restored
```

`motor_ctrl` releases after burst start — never holds across spin time.
Single `set_pin` or IRQ debounce: one scoped loan per flush.

## Buttons and IRQ

GPIO4 ISR must not perform I2C. Post `EVT_BUTTON_IRQ` to the app queue; the
handler acquires `EXPANDER`, reads P0/P1, releases. AW9523B INT mask is
written at boot (pre-`connsys_init`) and after wake.

## ADC / jam

Periodic `ADC` micro-loans in `motor_ctrl` every `[tune]` 100–500 ms while EN
is asserted. P1.7 selects motor path at burst start; samples use `ADC` profile
only. Jam stop: brief `EXPANDER` loan to deassert EN.

## Sleep and wake

Sleep: `EXPANDER` loan → peripherals off → `wifi_port.disconnect()` (see
[wifi-lifecycle.md](wifi-lifecycle.md#disconnect-sequence)); do not call full
`connsys_deinit` unless bench requires it.

Wake: GPIO4 / P0.3 IRQ → display boot sequence via arbiter (`DISPLAY` +
`EXPANDER`), not a second pre-`connsys_init()` hook.

## Verification

| Step | Assertion |
|------|-----------|
| Smoke | After `connsys_init()`, one `EXPANDER` loan → AW9523B ID `0x23` → release; broker still reachable |
| Display soak | Icon carousel ~500 ms cadence with MQTT heartbeat; no disassociation |
| Motor capstone | Max-grams dispense with MQTT; cumulative WFCI blockage < 5% |

Host fake records acquire/release order and hold duration; no single hold
> 10 ms except `FULL`.
