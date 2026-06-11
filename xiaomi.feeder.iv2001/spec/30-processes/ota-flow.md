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
persistent tasks to free RTOS stack RAM (~100–200 KB free at runtime; the
download worker needs `[tune]` 12 KB stack). `[design]`

| Task | Action during OTA window |
|------|--------------------------|
| `mqtt_io` | Disarm reconnect, disconnect broker session (task keeps running; broker buffers freed) |
| `remote_cli` | End active telnet session, close port 2323 listener, delete task to free stack (when `REMOTE_CLI_ENABLE`; recreated after failed OTA) |

UART0 CLI (`app_cli`) stays up. On download-worker spawn failure or any
download/verify/apply failure before reboot: resume suspended tasks (MQTT
reconnects when credentials are stored; telnet re-binds port 2323). On
successful apply: reboot — no resume. See
[uart-console.md](uart-console.md) § Remote telnet console.

### Steps

1. Publish `.../ota/status`: `{"state": "downloading", "pct": 0}`.
2. Suspend idle tasks (above).
3. HTTP(S) GET to provided URL.
4. Read in `[tune]` 4 KB chunks.
5. Write chunks to the inactive application bank (A/B layout).
6. Publish progress every `[tune]` 5 % (e.g. at 5, 10, 15 … 100 %).
7. Enforce maximum image size (partition size minus header). Abort if exceeded.

HTTPS: supported if `mqtt/tls` is enabled and mbedTLS RAM budget permits.

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
4. Reboot.

## Rollback

On boot after OTA:

1. Increment `system/boot_count` in NVDM.
2. Attempt Wi-Fi + MQTT connect.
3. If no successful MQTT connection within `[tune]` 60 s:
   - Mark current bank as bad.
   - Revert bootloader pointer to previous bank.
   - Reboot into known-good image.
4. On successful MQTT connect: clear boot_count, confirm new image.

## Error handling

| Failure | Action |
|---------|--------|
| HTTP connection failed | Abort, publish `"download_failed"` |
| Download interrupted | Abort, publish `"download_failed"`, inactive bank discarded |
| Verification failed | Abort, publish `"verify_failed"`, do not apply |
| Image too large | Abort mid-download, publish `"image_too_large"` |
| Post-apply crash loop | Rollback to previous bank or UART recovery |

## UART0 recovery (last resort)

- UART0 (GPIO21 TX, GPIO22 RX) always available for serial flash programming.
- Application CLI commands (`bank`, `wifi`) are defined in
  [uart-console.md](uart-console.md).
- MAC is in efuse; Wi-Fi config set at runtime — fully corrupted flash is
  recoverable without losing device identity.
- Not an OTA path; requires physical access to module pads.
