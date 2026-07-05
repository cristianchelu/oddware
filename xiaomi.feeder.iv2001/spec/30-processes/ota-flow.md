# OTA update flow

serves:
  - ../20-stories/updates.md

## Trigger

User publishes to `.../cmd/ota`:
```json
{"url": "http://192.168.1.100:8080/firmware.bin", "sha512": "<128 hex chars>"}
```

The `sha512` field is optional; when present it must match the SHA-512 of the
downloaded image bytes.

Device validates URL (non-empty, http:// or https:// scheme).

## Pre-flight checks

Firmware enforces URL validation and duplicate-OTA rejection. Dispense
gating is specified below but not enforced until the dispense supervisor
exists (`dispense-cycle.md`).

| Check | Action on failure | Enforced |
|-------|-------------------|----------|
| Dispense in progress? | Reject OTA, publish error: `"busy_dispense"` | No — requires `dispense-cycle.md` |
| URL scheme valid? | Reject, publish error: `"invalid_url"` | Yes |
| Already updating? | Reject, publish error: `"already_in_progress"` | Yes |

## Download

### Pre-download memory reclaim

Before spawning the download worker, firmware suspends non-essential
persistent tasks to free RTOS stack RAM. The FreeRTOS heap is `[tune]`
192 KB (`freertos_heap_192k.patch` on `minicli/inc/FreeRTOSConfig.h`); the
download worker needs `[tune]` 12 KB stack. `[design]`

| Task | Action during OTA window |
|------|--------------------------|
| Admin HTTP (`httpd`) | Stop LAN web UI when `WEB_UI_ENABLE=y`; restart after failed OTA when STA ready — see [web-ui.md](web-ui.md) |
| `remote_cli` | End active telnet session, close port 2323 listener, delete task to free stack (when `REMOTE_CLI_ENABLE`; recreated after failed OTA) |
| `app_cli` | Suspend UART0 console task and delete to free stack; recreated after failed OTA |
| `wifi_sta` | Suspend connect worker and delete to free stack; recreated after failed OTA |
| `mqtt_io` | Disarm reconnect, disconnect broker session (task keeps running; broker buffers freed) |

On download-worker spawn failure or any download/verify/apply failure before
reboot: resume suspended tasks in reverse order (admin HTTP when enabled,
MQTT, `wifi_sta`, `app_cli`, `remote_cli` when enabled). On successful apply:
reboot — no resume. See [uart-console.md](uart-console.md) § Remote telnet
console and [web-ui.md](web-ui.md).

### Steps

1. Publish `.../ota/status`: `{"state": "downloading", "pct": 0}`.
2. Suspend idle tasks (above).
3. HTTP(S) GET to provided URL.
4. Read in `[tune]` 4 KB chunks.
5. Write chunks to the inactive application bank (A/B layout).
6. Publish progress every `[tune]` 5 % (e.g. at 5, 10, 15 … 100 %).
7. Enforce maximum image size (partition size minus header). Abort if exceeded.

HTTPS: supported if `mqtt/tls` is enabled and mbedTLS RAM budget permits.

### Internal progress phases

The OTA port reports finer-grained `ota_status_t` values than MQTT exposes.
`PREPARING` and `CONNECTING` are internal only — `ota_client` still publishes
`"state": "downloading", "pct": 0` for both. Panel feedback during these
phases: [display-presentation.md](display-presentation.md) § OTA indicator.

| Internal status | When reported |
|-----------------|---------------|
| `PREPARING` | Download worker task starts (MQTT suspend already done in `start`) |
| `CONNECTING` | After pre-download settle, immediately before HTTP Range download |
| `DOWNLOADING` | First HTTP body bytes; `pct` 0–100 during transfer |
| `VERIFYING` | Download complete; SHA-512 / flash verify |
| `APPLYING` | Bank swap pending; reboot follows |
| `ERROR` | Any failure before reboot |

## Verification

After full download:

1. Read the A/B control block's expected hash (see
   [partition-layout.md](../40-architecture/partition-layout.md#ab-control-block)).
2. Compute SHA-512 over the written bank contents (LinkIt dual-image FOTA
   format). `[design]`
3. Compare against the expected hash from the OTA manifest.
4. On mismatch: abort, publish error `"verify_failed"`, do not swap banks.

No signature verification in v1 (no PKI infrastructure). `[design]`

## Flash layout

Total flash: 2 MB. **A/B dual-bank:** two application slots; a custom
bootloader boots one bank at a time. Download targets the inactive bank;
the running image is untouched until verification succeeds. `[design]`

## Apply

1. Publish `.../ota/status`: `{"state": "applying"}`.
2. Mark the verified inactive bank as the boot target.
3. Set boot flag for new image.
4. Reboot (see [Pre-reboot teardown](#pre-reboot-teardown)).

### Pre-reboot teardown

Before WDT reboot into the new bank:

1. Disconnect AP (`wifi_connection_disconnect_ap`) — sends deauth to the
   access point so it can clean up the STA entry immediately.
2. Wait `[tune]` 500 ms for the disconnect to propagate.
3. Disable I-cache; trigger `hal_sys_reboot()`.

The N9 coprocessor is force-reset at the next boot by
`connsys_force_n9_reset.patch` (assert `CONNSYS_SW_RST = 0x00` + 5 ms
before the existing MCU release in `_connsys_init_activate_mcu`). This
ensures N9 RAM is cleared regardless of WDT warm reboot state.

### Known limitation: bank-B boot delay

Canonical description: [wifi-lifecycle.md](wifi-lifecycle.md) § Bank-B boot delay.

Booting from flash bank B (OTA apply, manual `bank switch`, or cold start
when bank B is active) exhibits a ~30 s gap with no UART progress while the
N9 coprocessor is idle, then Wi-Fi association and DHCP complete in a few
more seconds (~42 s total to `EVT_WIFI_STA_READY` on bench). `[probe]`

During the gap the display shows no Wi-Fi icon (connect task is blocked in
`wait_ready`; `EVT_WIFI_STA_CONNECTING` was already posted). This is **not**
OTA-download-specific — any bank-B boot shows the same freeze.

Subsequent WPA 4-way handshake may log MIC Different on msg 3, resolved
internally by M3 reinstall attack skip. This is a fixed internal timeout
in the prebuilt N9 ROM (`libwifi_mt7682_ram.a`). Tested mitigations that did
**not** resolve it:

- Force N9 SW reset at boot (clears RAM / PMKSA — still ~30 s)
- Preserve STA NVDM credentials through reboot (still ~30 s)
- Skip pre-reboot `disconnect_ap` (still ~30 s)
- Credential-before-radio connect order (`set_credentials` → `radio_up` →
  `arm_connect`) `[probe]` 2026-06-16 — still ~42 s to `STA ready` on
  `bank switch` to B

Bank-A boots after B→A hop reach `STA ready` in ~1 s. Cold boots when bank A
is active connect in ~2 s. Any bank-B boot is the slow path (~42 s on bench);
acceptable for monthly OTA events. `[probe]`

### Active bank on MQTT

Every `.../ota/status` JSON includes `"bank": "A"` or `"B"` — the application
partition the bootloader will run. Bench OTA scripts (`tools/ota/`) read the
active bank from retained `ota/status`, not from `.../state`. See
[mqtt-protocol.md](mqtt-protocol.md) § OTA status.

## Slot health

One recovery path covers OTA apply and UART `bank switch`. The bootloader
auto-toggles the active bank after the strike limit when a slot stays
unverified. Slot confirmation means the firmware image boots and stays up —
not that MQTT is reachable.

### Control block

See [partition-layout.md](../40-architecture/partition-layout.md). Every
bank swap sets `unverified = 1` and `boot_attempts = 0`. Only crash-free
confirm clears `unverified` and `boot_attempts`.

### Bootloader: boot-attempt trap

Before bank selection on each boot, while `unverified` is set:

1. Increment `boot_attempts` and persist.
2. If `boot_attempts >=` `[tune]` `BOOT_MAX_ATTEMPTS` (3):
   - Log `boot attempt limit — switching bank`.
   - Toggle `active_flag` to the other bank.
   - Set `boot_attempts = 0`; keep `unverified = 1`.
   - Persist.

Then run existing header validation and cross-bank fallback. Bank validity
uses a vector-table scan over the first `[tune]` 64 KB (4-byte steps) —
the same probe used pre-swap during OTA verify — not the 8-byte header
probe alone.

There is no application-side timeout revert. Resets before confirm are
handled by the bootloader strike counter.

### Application: crash-free confirm

On boot, when `unverified` is clear, return immediately (steady-state boots).

When `unverified == 1`:

1. Increment `system/boot_count` in NVDM (diagnostics).
2. Start `[tune]` 60 s uptime timer.
3. On expiry without intervening reset: clear `unverified`, `boot_attempts`,
   and `boot_count` in the control block / NVDM.

Poll via the app timer tick (`ota_slot_health_poll_ms()`). MQTT connect is
not part of slot confirmation.

When a software watchdog is present (`power-state-machine.md`), a hang
during the confirm window feeds the bootloader strike path via WDT reset;
the confirm timer and WDT are independent mechanisms.

### Recovery layers

| Layer | Trigger | Location |
|-------|---------|----------|
| Boot attempt counter | Reset before 60 s confirm while `unverified` | Bootloader — 3 strikes, auto bank toggle |
| Crash-free confirm | 60 s uptime without reset while `unverified` | Application — clears `unverified` |
| Vector-table scan | Invalid vector table in first 64 KB | Bootloader — reject bank before jump |

### Dev deploy paths

| Workflow | UART required |
|----------|---------------|
| OTA when device is online | No |
| Iteration on a confirmed slot | No |
| Partial CODA flash, bootloader update, both slots bad | Yes |

Guardrails:

- Primary deploy: `mqtt-ota.sh` when MQTT is available.
- CODA / `iot-flash.sh`: flash bootloader + both banks from
  `flash_download.cfg`.
- Do not `bank switch` to a partially written slot; complete the
  inactive-bank write or use OTA.

## Error handling

| Failure | Action |
|---------|--------|
| HTTP connection failed | Abort, publish `"download_failed"` |
| Download interrupted | Abort, publish `"download_failed"`, inactive bank discarded |
| Verification failed | Abort, publish `"verify_failed"`, do not apply |
| Image too large | Abort mid-download, publish `"image_too_large"` |
| Post-apply crash loop | Bootloader bank toggle after 3 strikes; UART recovery if both slots fail |

## UART0 recovery (last resort)

- UART0 (GPIO21 TX, GPIO22 RX) always available for serial flash programming.
- Application CLI commands (`bank`, `wifi`) are defined in
  [uart-console.md](uart-console.md).
- MAC is in efuse; Wi-Fi config set at runtime — fully corrupted flash is
  recoverable without losing device identity.
- Not an OTA path; requires physical access to module pads.
