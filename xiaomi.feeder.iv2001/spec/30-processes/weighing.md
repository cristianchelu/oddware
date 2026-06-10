# Weighing

serves:
  - ../20-stories/monitoring.md
  - ../20-stories/feeding.md

## Weigh driver boundary

The weigh stack (`weight_port` → driver → CS1270) is **stateless** except for
NVDM calibration (`calib/zero`, `calib/span_g`, `calib/span_raw`). It answers
one question:

> **How many grams of food are in the bowl right now?**

| Owned by weigh driver | Owned by higher layers (dispense supervisor, monitoring) |
|--------------------------|---------------------------------------------------------|
| `read_grams` → absolute food g (empty bowl = 0) | `bowl_before` / `bowl_after` snapshots around events |
| `calibrate_zero` / `calibrate_span` → NVDM | `grams_delivered`, `grams_eaten`, `eaten_today` |
| CS1270 power rail, UART query, host cal math | Dispense compensation loops, MQTT publish cadence |
| `read_raw_grams` (bench / debug) | Display mode, midnight reset of eaten-today |

Higher layers compute **deltas** by calling `read_grams()` at two moments — they
do not ask the driver to remember a zero point:

```text
grams_eaten   = bowl_before − bowl_after    # pet ate
grams_added   = bowl_after  − bowl_before    # dispense
```

No runtime tare, session offset, or on-chip zero command in this layer. CS1270
on-chip cal and runtime zero (`F6 6F EE`) are documented in
[weigh-assp-cs1270.md](../10-hardware/components/weigh-assp-cs1270.md) but
**not used** — they would fight host-side NVDM cal.

## CS1270 lifecycle

The weigh ASSP is power-gated to save current when not in use.

| Phase | Action | Pin / Bus |
|-------|--------|-----------|
| Power on | EXPANDER micro-loan → set P0.2 high → release; `[tune]` 50 ms rail settle | P0.2 |
| | Must **not** call `gpio_expander_bootstrap` — that resets P0.5 and turns the display off | |
| Boot settle | Wait `[tune]` 1100 ms **with no WFCI loan held** (WFCI/SPI restored) | — |
| Init | Under `WEIGH` loan: poll query until not warming | UART2 |
| Sample | Weight query (`CA C2 EE`) | UART2 |
| Power off | EXPANDER loan → set P0.2 low | P0.2 |

UART frames and timing: [weigh-assp-cs1270.md](../10-hardware/components/weigh-assp-cs1270.md).

Power-off only when idle sampling is suspended (sleep mode, extended inactivity).
Do **not** toggle P0.2 while a UART transaction is in progress.

## Power-on sequencing

After `connsys_init()`, contested pins require WFCI bus loans
([wfci-bus-arbitration.md](wfci-bus-arbitration.md)):

1. **EXPANDER micro-loan** — assert P0.2 high; `[tune]` 50 ms rail settle; release
   (outputs latch).
2. **Delay** `[tune]` 1100 ms with WFCI restored — no bus loan held (CS1270 boot
   mode; stalls SPI for ~1 s if done inside a loan).
3. **WEIGH loan** — UART2 init, query or cal capture, release.

Keep CS1270 powered between reads during an active session; power off only
when entering sleep or extended idle.

## Calibration

2-point calibration is performed in host firmware and persisted in NVDM on
external NOR flash (`calib/zero`, `calib/span_g`, `calib/span_raw`). CS1270
on-chip EEPROM calibration (`3A 4C`) is **not** used — coefficients are lost
when the ASSP is power-cycled via P0.2.

| Step | Mechanism | Trigger |
|------|-----------|---------|
| Zero capture | Query raw count with bowl **removed** → save `calib/zero` | UART `weigh cal zero`, MQTT `cmd/calibrate {"action":"zero"}` |
| Span capture | Query raw count with provided bowl installed (350 g) → save `calib/span_g` + `calib/span_raw` | UART `weigh cal span`, MQTT `cmd/calibrate {"action":"span"}` |

Span workflow: remove bowl → `weigh cal zero` → install provided bowl →
`weigh cal span`. Span mass is fixed at 350 g (`[product]`). Food grams:

`food_g = (raw − zero) × 350 / (span_raw − zero) − 350`

The CS1270 must return a weight frame (`00`/`01` CMD3) during capture and
read. Factory ASSP linearization supplies the raw counts; host cal maps them
to true grams (see **Weigh driver boundary** above).

Factory reset erases the `calib` namespace.

## Sampling modes

| Mode | Interval | Active when |
|------|----------|-------------|
| Idle | `[tune]` 5 s | No dispense active; report current bowl weight |
| Dispense | `[tune]` 500 ms | Dispense in progress; feed compensation loop |
| Off | — | Sleep mode or deep power save |

Transition between modes is driven by the dispense supervisor.

## UART2 serialized access

CS1270 uses command/response on UART2. Only one client may communicate at a
time:

- Idle sampling and dispense compensation must not overlap.
- A serialization mechanism (mutex or equivalent scheduling) guards UART2 access.
- Baud rate: 9600 8N1 default `[tune]`; 115200 fallback if bench proves needed.

## Data model

### From `read_grams` (weigh driver)

| Value | Type | Unit | Description |
|-------|------|------|-------------|
| `bowl_weight` | int | grams | Food in bowl now (empty bowl = 0 g); direct `read_grams` result |

### Derived elsewhere (not stored in weigh driver)

| Value | Type | Unit | Owner | Description |
|-------|------|------|-------|-------------|
| `eaten_today` | int | grams | Monitoring | Cumulative consumption since midnight; uses bowl snapshots + dispense history |
| `last_dispense_actual` | int | grams | Dispense supervisor | `bowl_after − bowl_before` for the completed cycle |

MQTT `.../weight` publishes `bowl_g` (from `read_grams`) and `eaten_today_g`
(from monitoring). See [weight-compensation.md](weight-compensation.md) and
[dispense-cycle.md](dispense-cycle.md) for how supervisors snapshot reads.

## Error handling

| Condition | Response |
|-----------|----------|
| CS1270 no response after power-on | Retry `[tune]` 3 times, then report sensor fault via MQTT |
| Implausible reading (> 5000 g or < −100 g) | Discard sample, log, continue |
| Consistent negative drift over time | Publish recalibration suggestion via MQTT |
| UART timeout mid-transaction | Abort read, retry after `[tune]` 100 ms |
| Host cal incomplete | Report not calibrated; prompt `weigh cal zero` / `span` |
| CS1270 not returning weight frames | Report I/O fault; check ASSP power and warming |
