# Application event loop

serves:
  - ../20-stories/monitoring.md
  - ../20-stories/display.md

## Role

All application business logic runs in one FreeRTOS task (`app`) that drains
`app_event_q`. Producers (Wi-Fi STA task, MQTT I/O task, software timers)
post typed events only; they do not call display indicators, MQTT connect
policy, OTA command dispatch, or weight idle sampling directly. `[design]`

Event type names and queue depth: [task-model.md](../40-architecture/task-model.md).

## Dispatch policy

Each `app` wake drains **all** pending queue items before blocking again.
This keeps indicator and display updates aligned with the latest session
state when several events arrive in one timer period.

Host tests call `app_step()` with the same dispatcher and a FIFO fake queue
(no FreeRTOS).

## Phase 2 event table

| Event | Producer | Consumer action |
|-------|----------|-----------------|
| `EVT_APP_BOOT` | `app_start()` once after queue + timers exist | Enter default display mode **weight**; start weight boot FSM (rail on → `[tune]` 1100 ms settle → first `read_grams`) |
| `EVT_WIFI_STA_CONNECTING` | `wifi_sta_request_connect()` | `display_wifi_indicator_connecting()` |
| `EVT_WIFI_STA_READY` | `wifi_sta.c` on DHCP OK | `display_wifi_indicator_connected()`; `time_sync_on_wifi_ready()`; if MQTT broker config stored → `mqtt_client_request_connect()` |
| `EVT_WIFI_STA_FAILED` | STA connect or IP failure | `display_wifi_indicator_off()` |
| `EVT_WIFI_STA_AP_MODE` | `provision.c` / `provision_wifi_try.c` when AP portal active | `display_wifi_indicator_ap_mode()` |
| `EVT_MQTT_SESSION` | `mqtt_client_request_connect()` and `mqtt_client_step()` when derived session phase changes | Map phase → `display_mqtt_indicator_*` (see [mqtt-protocol.md](mqtt-protocol.md) § Session display) |
| `EVT_MQTT_CONNECTED` | `mqtt_client_do_connect()` success | `app_mqtt_on_connected()` — enqueue idle `ota/status`, retained telemetry snapshots (mains, hopper, battery, config), schedule HA discovery, `mqtt_schedule_connect_snapshot()`; no display side effect |
| `EVT_MQTT_MESSAGE` | MQTT message callback | Heap-copy topic + payload; `mqtt_route_classify` → dispatch (`cmd/ota`, `cmd/dispense`, `cmd/schedule/*`, other routes stub) |
| `EVT_DISPLAY_TICK` | `[tune]` 50 ms soft timer | Idle `try_read_grams` (2 Hz, rate-limited) + scene sync + `display_presentation_tick(now_ms)` + `button_input_poll(now_ms)` + `button_gesture_step(now_ms)` + drain transitions/gestures (includes P0.4 reset sampling) + `hopper_input_poll(now_ms, background_enabled)` (mains-only 60 s background) + `hopper_level_poll()` + drain `hopper_level_pop_transition` → `mqtt_hopper_sync` in one handler |
| `EVT_TIMER_TICK` | `[tune]` 500 ms soft timer | `time_sync_poll(now_ms)`; `schedule_poll(now_ms)`; `ota_slot_health_poll_ms()`; weight boot FSM only (coalesced when queue busy) |
| `EVT_BUTTON_IRQ` | GPIO4 ISR (AW9523B INT) | `button_input_notify_irq(now_ms)` then `button_input_poll(now_ms)`; IRQ-backed buttons ignore samples until `now_ms` ≥ IRQ time + `[tune]` 50 ms |

The `app` task waits on the queue with a `[tune]` 50 ms timeout; on timeout it
runs the same `EVT_DISPLAY_TICK` handler locally (weight sample + presentation
refresh) so display stays live when the timer daemon or queue is backlogged
during MQTT connect.

`app_event_post` coalesces duplicate `EVT_DISPLAY_TICK`, `EVT_TIMER_TICK`, and
`EVT_BUTTON_IRQ` entries (at most one of each pending). Queue depth is `[tune]` 32 items — see
[task-model.md](../40-architecture/task-model.md).

## Coexistence with MQTT connect

The TCP/MQTT handshake runs on a short-lived `mqtt_cn` worker at priority
**below** `app`; `mqtt_io` polls completion every `[tune]` 50 ms and never
blocks on `ConnectNetwork` / `MQTTConnect`. Session phase **connecting** posts
to `app_event_q` from `mqtt_client_request_connect()` before the worker
starts — the orange lightbar and weight digits remain schedulable while the
handshake runs.

| Concern | Owner | Rule |
|---------|-------|------|
| Broker TCP + MQTT CONNECT | `mqtt_cn` worker (ephemeral) | Blocking network calls stay off `mqtt_io` and off `app` |
| Session phase → lightbar scene | `app` on `EVT_MQTT_SESSION` | Indicator helpers update scene only — no `display_presentation_refresh()` |
| TM1637 physical refresh | `app` on `EVT_DISPLAY_TICK` (timer or local heartbeat) | `try_show_grids`; skip frame on `PORT_ERR_BUSY`, retry next tick |
| Idle bowl grams | `app` on same `EVT_DISPLAY_TICK` turn | `try_read_grams` at `[tune]` 500 ms; `PORT_ERR_BUSY` keeps last sample; I/O errors clear it |
| Bowl error pictograph + weight digits | `app` on same `EVT_DISPLAY_TICK` turn | `app_weight_sync_display_scene()` — `bowl_error_eval` → `display_bowl_error_indicator_sync` + digit mode (`---`, `-  `, or clamped grams) |
| MQTT bowl weight | `app` on same weight sync turn | `mqtt_bowl_weight_sync()` — change-driven publish to `.../bowl_weight`; not on every 500 ms sample |
| Weight boot settle | `app` on `EVT_TIMER_TICK` | `boot_begin` / `boot_poll` — no multi-second `vTaskDelay` in `app` |

**Regression constraints:** synchronous `mqtt->connect()` on `mqtt_io` or `app`
starves the panel; synchronous `display_presentation_refresh()` from indicator
helpers doubles WFCI `DISPLAY` loans; idle weight sampling on `EVT_TIMER_TICK`
alone misses samples when that event is dropped; clearing the cached gram
reading on `PORT_ERR_BUSY` blanks the scene while TM1637 may still show stale
pixels.

| `EVT_DISPENSE_REQUEST` | `dispense_submit_*` (UART CLI, button, MQTT, schedule) | Payload: `{kind, target}` — dispense supervisor starts job, `request_burst(N, 8000)` for N portions |
| `EVT_BURST_DONE` | `motor_ctrl` | Dispense supervisor completes job when active; UART `dispense done`; `hopper_level_notify_dispense_complete()` (forwards to `hopper_input`); `schedule_on_dispense_complete()` when source is schedule; deferred bowl-gram resample on next `EVT_DISPLAY_TICK`; otherwise motor CLI handlers only |
| `EVT_MOTOR_FAULT` | `motor_ctrl` | Dispense supervisor aborts job when active; UART `dispense fault: stuck`, `motor park fault: stuck`, or `motor fwd/rev fault: stuck` per active CLI command; `hopper_level_notify_dispense_complete()` when dispense job was active |
| `EVT_PARK_DONE` | `motor_ctrl` | UART `motor park done` when park was active |
| `EVT_TIMED_RUN_DONE` | `motor_ctrl` | UART `motor fwd ok` or `motor rev ok` when the matching bench timed run was active |

`dispense`, `motor park`, and `motor fwd` / `motor rev` UART commands are fully
non-blocking on the CLI task: started/busy lines print immediately; done/fault
lines print when `app` handles the matching completion event.

`dispense_poll()` is called on each `EVT_DISPLAY_TICK` and `EVT_TIMER_TICK` as a
hook for future job-level timeouts; it does **not** complete the job from
`motor_port.is_active()`. A dispense **job** stays active from request accept
until the supervisor finishes it explicitly (`EVT_BURST_DONE`, `EVT_MOTOR_FAULT`,
or future compensate / cancel paths). Motor may stop and restart within one job
(e.g. post-batch weigh settle); the dispensing pictograph blinks for the whole
job — see [display-presentation.md](display-presentation.md) § Dispensing
indicator.

While a dispense job is active, idle bowl-gram sampling on `EVT_DISPLAY_TICK`
is suspended so WFCI / weigh UART work cannot block completion handling.

The app task drains its event queue in two passes: motor/dispense completion
events (`EVT_BURST_DONE`, `EVT_MOTOR_FAULT`, `EVT_DISPENSE_REQUEST`, …) before
`EVT_DISPLAY_TICK`. At most one display tick is handled per drain batch.

Reserved for later phases (handlers are no-ops when posted): gram-targeted
compensated dispense, dispense FIFO queue, MQTT dispense dispatch,
provisioning submit, OTA progress/complete.

## MQTT session phase payload

`EVT_MQTT_SESSION` carries a phase enum only — no display calls inside
`mqtt_client.c`:

| Phase | Indicator |
|-------|-----------|
| off | `display_mqtt_indicator_off()` |
| connecting | `display_mqtt_indicator_connecting()` |
| connected | `display_mqtt_indicator_connected()` |
| error | `display_mqtt_indicator_error()` |

Derivation rules match [display-presentation.md](display-presentation.md) § MQTT
indicator (armed/suspended, Wi-Fi ready, connect pending, backoff armed,
broker connected).

## Weight boot FSM

Non-blocking state across `EVT_TIMER_TICK` (`[tune]` 500 ms / 2 Hz):

1. `EVT_APP_BOOT` arms weight boot (display mode **weight**); rail bring-up waits for the first
   `EVT_TIMER_TICK` so WFCI / Wi-Fi SPI is not contended during `main()` bring-up.
2. First `EVT_TIMER_TICK`: `boot_begin()` (rail on only). Further ticks call `boot_poll()` until
   `[tune]` 1100 ms CS1270 settle elapses (non-blocking — see [weighing.md](weighing.md)); then
   first blocking `read_grams()` and digit scene sync.
3. Idle: `try_read_grams()` on `EVT_DISPLAY_TICK` at `[tune]` 500 ms (2 Hz), immediately
   followed by `display_presentation_tick` so sample + `try_show_grids` share one app
   handler turn. `PORT_ERR_BUSY` keeps the last good sample (WFCI contended during MQTT
   connect); I/O errors clear it. `[tune]` starting value for bench characterization.

Default display mode is **weight** (hardcoded in `app` — no NVDM `display/mode`,
no `cmd/display` in this phase). Digit grids: uncalibrated → `---g`; calibrated
bowl missing → `-  g`; calibrated with a sample in range → grams; calibrated but
no sample yet (or read I/O fault) → blank digits with `GRAM` unit — not `---`.
Bowl-error pictograph policy: [display-presentation.md](display-presentation.md)
§ Bowl error indicator.

## Intentional direct-wired exceptions

| Path | Reason |
|------|--------|
| UART `display` / `weigh` CLI | User-initiated bench commands, not lifecycle policy |
| `mqtt_cn` connect worker | Subscribe + retained `connection` publish only; post-connect topics use [mqtt_outbox](mqtt-protocol.md#publish-path) |
| OTA download progress → MQTT publish | Progress callback enqueues `ota/status`; `mqtt_io` drains |

## Verification

Host tests in `firmware/test/test_app.c` and `firmware/test/test_mqtt_client.c`
exercise dispatcher routing, MQTT session indicators, queue coalescing, and
weight sampling during the connecting phase without FreeRTOS. Cross-compile links
`app_event_port.c` and `app_task.c`; host tests link `fake_app_event_q.c`
(mirrors firmware coalescing behavior).
