# Weighing

serves:
  - ../20-stories/monitoring.md
  - ../20-stories/feeding.md

## CS1270 lifecycle

The weigh ASSP is power-gated to save current when not in use.

| Phase | Action | Pin / Bus |
|-------|--------|-----------|
| Power on | Set P0.2 high (AW9523B, I2C @ 0x58) | P0.2 |
| Init | Send configuration commands via UART2 (GPIO11 TX, GPIO12 RX), wait for ready | UART2 |
| Tare | Set current load as zero reference | UART2 command |
| Sample | Periodic weight reads at configured interval | UART2 command |
| Power off | Set P0.2 low | P0.2 |

Power-off only when idle sampling is suspended (sleep mode, extended inactivity).
Do **not** toggle P0.2 while a UART transaction is in progress.

## Calibration procedure

| Step | Action | Trigger |
|------|--------|---------|
| Tare | Record current load as zero offset; store in NVDM `calib/tare` | Boot, MQTT `cmd/calibrate {"action":"tare"}`, button combo |
| Span | Place known weight on bowl, compute scale factor; store in NVDM `calib/span` | MQTT `cmd/calibrate {"action":"span","g":200}` |

Calibration values survive power cycles. On boot, load `calib/tare` and `calib/span`
from NVDM before first read.

## Sampling modes

| Mode | Interval | Active when |
|------|----------|-------------|
| Idle | `[tune]` 5 s | No dispense active; report current bowl weight |
| Dispense | `[tune]` 500 ms | Dispense in progress; feed compensation loop |
| Off | — | Sleep mode or deep power save |

Transition between modes is driven by the dispense supervisor.

## UART2 serialized access

CS1270 uses half-duplex command/response on UART2. Only one client may
communicate at a time:

- Idle sampling and dispense compensation must not overlap.
- A serialization mechanism (mutex or equivalent scheduling) guards UART2 access.
- Baud rate: `[tune]` likely 9600 or 115200 `[ds:CS1270]`.

## Data model

| Value | Type | Unit | Description |
|-------|------|------|-------------|
| `bowl_weight` | int | grams | Current weight on bowl (tared) |
| `eaten_today` | int | grams | Cumulative dispensed − current bowl delta since midnight |
| `last_dispense_actual` | int | grams | Grams added by most recent dispense |

Published to MQTT `.../weight`: `{"bowl_g": <bowl_weight>, "eaten_today_g": <eaten_today>}`.

## Error handling

| Condition | Response |
|-----------|----------|
| CS1270 no response after power-on | Retry `[tune]` 3 times, then report sensor fault via MQTT |
| Implausible reading (> 5000 g or < −100 g) | Discard sample, log, continue |
| Consistent negative drift over time | Publish recalibration suggestion via MQTT |
| UART timeout mid-transaction | Abort read, retry after `[tune]` 100 ms |
