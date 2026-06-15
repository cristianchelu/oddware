# Task model and inter-task communication

## Pattern: single application event loop

All application business logic runs in one FreeRTOS task context (`app`),
consuming events from a single queue (`app_event_q`). This eliminates
shared-state races across application code and makes the dispatcher
testable on the host without FreeRTOS. `[design]`

```
WiFi events ─────┐
MQTT rx ──────────┤
Button IRQs ──────┤     ┌──────────────┐
Timer expiry ─────┼────►│ app_event_q  │────► app (NORMAL)
OTA progress ─────┤     └──────────────┘       │
Provisioning ─────┘                            ├─► calls port functions
                                               ├─► updates state
                                               ├─► enqueues MQTT TX via mqtt_outbox
```

## SDK-owned tasks (do not modify)

The SDK creates these internally. Their priorities are fixed:

| Task | Priority | Role |
|------|----------|------|
| `net` | HIGH | WiFi network processing |
| `inband` | HIGH | WiFi CM4-to-N9 inband commands |
| `lwIP` | HIGH | TCP/IP stack |
| `SYSLOG` | 1 | Log output |
| `cli` | SOFT_REALTIME | UART command line |
| `dhcpd` | NORMAL | DHCP server (AP mode only) |
| Timer daemon | MAX-1 | FreeRTOS software timers |

## Application tasks

| Task | Priority | Stack | Lifetime | Role |
|------|----------|-------|----------|------|
| `app` | NORMAL | 4096 B | Persistent | Event loop: dequeues `app_event_t`, dispatches to handler, calls ports; local `[tune]` 50 ms display heartbeat when queue idle |
| `mqtt_io` | ABOVE_NORMAL | 4096 B | Persistent | MQTT orchestration: drain `mqtt_outbox` (sole post-connect publisher) then `MQTTYield()` when connected; starts `mqtt_cn` connect worker and polls every `[tune]` 50 ms while handshake runs; disconnected during OTA preflight |
| `mqtt_cn` | NORMAL − 1 | 4096 B | Ephemeral | Blocking `ConnectNetwork` + `MQTTConnect` + post-connect subscribe/publish; self-deletes on completion |
| `ota_dl` | NORMAL | 4096 B | Ephemeral | Spawned during OTA download only, posts progress to `app_event_q`, self-deletes on completion |
| `motor_ctrl` | HIGH | 2048 B | Persistent (when dispense features land) | Safety-critical motor I2C: receives fault notifications from ADC ISR, performs protective I2C writes, posts `EVT_MOTOR_FAULT` to `app_event_q` |
| `remote_cli` | NORMAL | `[tune]` 4096 B | Persistent when `REMOTE_CLI_ENABLE=y` | TCP listen/accept on port 2323; runs MiniCLI on the active socket; attaches/detaches telnet `app_log` mirror; all socket I/O in this task only; **deleted** during OTA preflight when enabled (recreated after failed OTA) |
| `app_cli` | NORMAL | 4096 B | Persistent | UART0 MiniCLI; polls UART for local override while a remote session is active; **deleted** during OTA preflight (recreated after failed OTA) |
| `wifi_sta` | NORMAL | 4096 B | Persistent | STA connect/disconnect worker; posts `EVT_WIFI_STA_*`; **deleted** during OTA preflight (recreated after failed OTA) |

`motor_ctrl` runs at HIGH priority -- above the WiFi `net` task -- because
mechanical damage from a stalled motor is a harder failure than a missed
WiFi packet. The jam-detection spec requires sub-millisecond response
(`jam-detection.md`); ISR-to-task notification + I2C write stays well under
that budget. `[design]`

### Bench HAL vs motor_ctrl

Bench UART `motor fwd <ms>` / `motor rev <ms>` enqueue timed runs on
`motor_ctrl` via `motor_port.request_timed_forward_ms` /
`request_timed_reverse_ms` — same non-blocking CLI pattern as `dispense` and
`motor park` (see [uart-console.md](../30-processes/uart-console.md) § motor
commands). There is no separate off/abort command.

`motor_ctrl` (HIGH) owns all motor I/O: product bursts/park (index IRQ, ADC jam
stop), bench timed runs (PH settle + EN spin only), and preemptive `stop`.
Operator abort is a `motor_ctrl` / product concern, not bench CLI.

## Event queue (`app_event_q`)

A single FreeRTOS queue of `app_event_t` -- a tagged union of event type
plus a small data payload. `[design]`

Queue depth: `[tune]` 32 items. `app_event_post` coalesces duplicate
`EVT_DISPLAY_TICK` and `EVT_TIMER_TICK` (at most one of each type pending).
`app_event_t` is kept small (type + union of a pointer or a few scalars).
Large payloads (MQTT message body, OTA chunk) are heap-allocated by the
producer; the consumer frees after processing.

Canonical event-type dispatch table: [app-event-loop.md](../30-processes/app-event-loop.md)
§ Phase 2 event table.

### Event types

| Event | Producer | Consumer action |
|-------|----------|-----------------|
| `EVT_WIFI_CONNECTED` | WiFi event handler | Update state, attempt MQTT connect |
| `EVT_WIFI_DISCONNECTED` | WiFi event handler | Start reconnect timer |
| `EVT_MQTT_CONNECTED` | `mqtt_io` | `app_mqtt_on_connected()` — OTA rollback confirm, enqueue idle `ota/status`, schedule HA discovery |
| `EVT_MQTT_DISCONNECTED` | `mqtt_io` | Start backoff reconnect |
| `EVT_MQTT_MESSAGE` | `mqtt_io` | Dispatch by topic (cmd/dispense, cmd/ota, etc.) |
| `EVT_MQTT_RECONNECT_TICK` | Software timer | Attempt MQTT reconnect |
| `EVT_OTA_PROGRESS` | `ota_dl` | Enqueue OTA status via mqtt_outbox |
| `EVT_OTA_COMPLETE` | `ota_dl` | Verify image, swap banks, reboot |
| `EVT_OTA_ERROR` | `ota_dl` | Enqueue error status via mqtt_outbox |
| `EVT_PROVISION_SUBMIT` | HTTP server callback | Validate creds, save to NVDM, reboot |
| `EVT_TIMER_TICK` | Software timer | Periodic housekeeping |
| `EVT_DISPLAY_TICK` | Display presentation timer (`[tune]` 50 ms) | `display_presentation_tick()` — blink phases and animation frames |
| `EVT_DISPENSE_REQUEST` | MQTT cmd or button | Dispense supervisor starts job, `request_burst(N, 8000)` for N portions |
| `EVT_BURST_DONE` | `motor_ctrl` | Read weight, update display, send next burst or park |
| `EVT_MOTOR_FAULT` | `motor_ctrl` | Abort dispense, display error, publish fault |
| `EVT_PARK_DONE` | `motor_ctrl` | Publish completion, update display |
| `EVT_BUTTON_IRQ` | GPIO4 ISR (AW9523B INT) | Defer expander read to `app`; see `button-handling.md` |
| `EVT_BUTTON_GESTURE` | `app` after `button_gesture` classifies hold | Gesture routing (dispense, sleep, provisioning); bring-up drains synchronously on display tick |

## ISR-to-task communication

Two tiers of ISR response:

### Non-safety ISRs

Button press, timer expiry, WiFi event callbacks: use
`xQueueSendFromISR()` to post to `app_event_q`. The ISR captures type +
raw value and defers all logic to `app`.

### Safety-critical ISRs

Motor-load ADC threshold crossing on GPIO17 (direct MT7682 pin): use
`xTaskNotifyFromISR()` to wake `motor_ctrl` directly. Single-word
notification, zero-copy, faster than queue. The ISR latches a fault flag;
`motor_ctrl` performs the protective I2C write (disable EN on P0.1);
`app_event_q` gets the reporting event afterward.

This two-tier design exists because motor control pins are on the AW9523B
GPIO expander (I2C) -- the ISR itself cannot toggle them, but it must wake
the highest-priority task that can. `[design]`

## Software timers

FreeRTOS software timers handle periodic work: MQTT reconnect backoff,
watchdog petting, status publish throttling. Timer callbacks run in the
timer daemon context and must not block. Each callback posts a typed event
(e.g. `EVT_MQTT_RECONNECT_TICK`) to `app_event_q`.

## Bus topology during dispense

Three peripherals interact during a dispense cycle, on three separate
buses. There is no data-path contention between them:

| Bus | Peripherals | Owner during motor run |
|-----|-------------|------------------------|
| **I2C** (AW9523B @ 0x58) | Motor EN/PH (P0.0/P0.1), index IR (P0.6/P0.7), weight scale power (P0.2), display power (P0.5) | `motor_ctrl` (HIGH) for dispense, park, and bench timed runs |
| **UART2** (GPIO11/12) | CS1270 weight scale reads | `app` -- reads between bursts only |
| **GPIO bit-bang** (GPIO1/GPIO13) | TM1637 display data | `app` -- refreshes between bursts (~1 ms critical section) |

### Dispense event flow

```
EVT_DISPENSE_REQUEST (from MQTT cmd or button)
  -> init state machine, send MOTOR_CMD_BURST to motor_ctrl
EVT_BURST_DONE (from motor_ctrl, burst completed or index target reached)
  -> read weight (UART2, ~50 ms, no I2C contention)
  -> update display (bit-bang, ~1 ms)
  -> publish progress (MQTT)
  -> if more bursts: send next MOTOR_CMD_BURST
  -> else: send MOTOR_CMD_PARK
EVT_MOTOR_FAULT (from motor_ctrl, at any time during motor run)
  -> abort state machine, show error on display, publish fault
EVT_PARK_DONE (from motor_ctrl, auger at seal index)
  -> publish completion, update display
```

`app` never blocks on motor I/O. Between bursts, the ~50 ms weight UART
read is the longest blocking call, which is acceptable -- other events
queue and drain immediately after.

## Design rationale

- **No mutexes in application code.** All mutable application state lives
  in `app` context. Ports are called only from `app`. No concurrent access,
  no locking, no priority inversion.
- **Testable on host.** In host tests, the event queue is a simple FIFO
  array. Push events, call the dispatcher, assert state changes. No
  FreeRTOS dependency.
- **Extensible.** Adding a subsystem (weighing, display, scheduling) means:
  add an event type, add a handler case, write a test. The queue and
  dispatcher are unchanged.
- **`mqtt_io` owns post-connect TX.** It drains `mqtt_outbox` (one item per
  step, rate-spaced) then runs `MQTTYield()`. On message arrival it
  heap-copies topic+payload to `EVT_MQTT_MESSAGE` and posts to the queue.
  No business logic and no direct publish from `app` after connect.
- **OTA download is isolated.** `ota_dl` does blocking HTTP GET + flash
  writes. It posts progress events. `app` never blocks on I/O. Cancellation
  is via an abort flag that `ota_dl` checks between chunks.
- **Motor safety is orthogonal.** `motor_ctrl` handles the time-critical
  I2C path independently. The event loop handles the orchestration.
