# OTA update flow

serves:
  - ../20-stories/updates.md

## Trigger

User publishes to `.../cmd/ota`:
```json
{"url": "http://192.168.1.100:8080/firmware.bin"}
```

Device validates URL (non-empty, http:// or https:// scheme).

## Pre-flight checks

| Check | Action on failure |
|-------|-------------------|
| Dispense in progress? | Reject OTA, publish error: `"busy_dispense"` |
| URL scheme valid? | Reject, publish error: `"invalid_url"` |
| Already updating? | Reject, publish error: `"already_in_progress"` |

## Download

1. Publish `.../ota/status`: `{"state": "downloading", "pct": 0}`.
2. HTTP(S) GET to provided URL.
3. Read in `[tune]` 4 KB chunks (RAM-constrained: ~100–200 KB free at runtime).
4. Write chunks to the inactive application bank (A/B layout).
5. Publish progress every `[tune]` 5 % (e.g. at 5, 10, 15 … 100 %).
6. Enforce maximum image size (partition size minus header). Abort if exceeded.

HTTPS: supported if `mqtt/tls` is enabled and mbedTLS RAM budget permits.

## Verification

After full download:

1. Read the A/B control block's expected hash (see
   [partition-layout.md](../40-architecture/partition-layout.md#ab-control-block)).
2. Compute SHA-512 over the written bank contents. The houndify SDK's
   dual-image FOTA uses SHA-512 for image integrity; we retain this
   algorithm. `[design]`
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
- MAC is in efuse; Wi-Fi config set at runtime — fully corrupted flash is
  recoverable without losing device identity.
- Not an OTA path; requires physical access to module pads.
