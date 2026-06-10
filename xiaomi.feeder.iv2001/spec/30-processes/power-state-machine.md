# Power state machine

serves:
  - ../20-stories/controls.md
  - ../20-stories/monitoring.md

## States

| State | Entry condition | Behavior |
|-------|-----------------|----------|
| **Normal** | Mains present (P1.1 = high) | Full operation, Wi-Fi on, display on, all subsystems active |
| **Battery** | Mains lost (P1.1 = low) | Reduced operation, Wi-Fi configurable, schedules continue |
| **Sleep** | User long-press P0.3 (~3 s) | Minimal draw, motor off, display off, Wi-Fi off |

## State transitions

```
          mains present (P1.1 high)
    ┌─────────────────────────────────┐
    │                                 │
    ▼                                 │
 ┌────────┐   mains lost (P1.1 IRQ)  │   ┌─────────┐
 │ Normal │ ────────────────────────► │   │  Sleep   │
 └────────┘                           │   └─────────┘
    │  ▲                              │      ▲  │
    │  │ mains present                │      │  │ button IRQ
    │  │                         ┌────────┐  │  │ (P0.3)
    │  └──────────────────────── │Battery │──┘  │
    │     long-press P0.3        └────────┘     │
    └───────────────────────────────────────────┘
```

### Mains detection

- P1.1 (AW9523B input + IRQ): high = mains present, low = battery. `[probe-needed]`
- IRQ on level change → immediate state transition.
- On mains loss: publish `power_source: battery` via MQTT (if connected).

### Normal → Battery

1. P1.1 IRQ fires (mains removed).
2. Optionally disable Wi-Fi per `power/battery_wifi` setting:
   - `on`: keep Wi-Fi connected. `[design]`
   - `off`: disconnect Wi-Fi immediately.
   - `scheduled_only`: connect Wi-Fi only around scheduled dispense times.
3. Continue executing schedules from battery power.

### Battery → Normal

1. P1.1 IRQ fires (mains restored).
2. Re-enable Wi-Fi if it was disabled.
3. Resume full operation.

### Any → Sleep

1. User long-presses rear power button (P0.3) for `[tune]` 3 s.
2. Abort any in-progress dispense gracefully (motor off, P0.1 low).
3. Power off display (P0.5 low).
4. Disconnect Wi-Fi.
5. Power off CS1270 (P0.2 low).
6. Disable motor-index IR LED (P0.6 low).
7. Enter minimal-draw state; only P0.3 IRQ and watchdog remain active.

### Sleep → Normal / Battery

1. Button IRQ on P0.3 (short press = wake).
2. Run boot sequence (see below).
3. Check P1.1 to determine whether Normal or Battery state.

## Boot sequence (power-on / reset / wake)

| Step | Layer | Action |
|------|-------|--------|
| 1 | — | Bootloader runs |
| 2 | — | Application init: HAL, clock tree, EPT GPIO |
| 3 | Presentation + display stack | Boot self-test (`display_boot_run`) **before** `connsys_init()` — AW9523B on I2C1 and WFCI SPI share GPIO12–16 `[probe]` |
| 4 | — | Wi-Fi firmware download (`connsys_init`) |
| 5 | — | Load config from NVDM |
| 6 | — | Attempt Wi-Fi connection (non-blocking) |
| 7 | `app` | Start scheduler; `app` task owns idle weight sampling and weight-mode display refresh at `[tune]` 500 ms (2 Hz) — see [app-event-loop.md](app-event-loop.md) |
| 8 | — | Ready state |

Boot self-test steps 3 mechanics: [display-driver.md](display-driver.md) § Boot self-test.
UART2 (CS1270) and ADC init stay deferred to later features.

## Watchdog

- Software watchdog timeout: `[tune]` 30 s.
- Main processing loop must pet the watchdog within this period.
- On watchdog reset: increment `system/boot_count` in NVDM, set
  `system/last_reset = watchdog`.
- After reconnect, report watchdog event via MQTT.
