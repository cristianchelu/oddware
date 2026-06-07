# Provisioning flow

serves:
  - ../20-stories/provisioning.md

## AP mode entry

Device enters Wi-Fi AP mode on:

- **First boot** — no stored Wi-Fi credentials in NVDM.
- **Factory reset** — pin-hole long press (7 s) clears all NVDM, reboots.
- **Re-provisioning** — pin-hole short press enters temporary AP.

## AP configuration

| Parameter | Value |
|-----------|-------|
| SSID | `PetFeeder-XXXX` (XXXX = last 4 hex chars of MAC) |
| Security | WPA2 with device-unique PSK (preferred) `[design]` |
| Fallback | Open AP if PSK derivation not implemented |
| IP address | 192.168.4.1 (standard AP gateway) |
| DHCP | Device runs DHCP server, assigns 192.168.4.x to clients |

PSK derivation: hash of serial number or MAC, printed on device label. `[design]`

## Captive portal

Minimal HTTP server at 192.168.4.1 serving a single-page form:

| Field | Type | Required | Default |
|-------|------|----------|---------|
| Wi-Fi SSID | text | yes | — |
| Wi-Fi password | password | no | (empty = open network; validation: [uart-console.md](uart-console.md#wi-fi-credential-rules)) |
| MQTT broker host | text | yes | — |
| MQTT broker port | number | no | 1883 |
| MQTT username | text | no | (empty = anonymous) |
| MQTT password | password | no | (empty) |
| Device ID | text | no | MAC-derived |

Form submission via HTTP POST. No JavaScript dependency — should work in
captive portal webviews on all platforms.

## Save + connect flow

On form submit:

| Step | Action | On failure |
|------|--------|------------|
| 1 | Validate inputs (Wi-Fi SSID/password per [uart-console.md](uart-console.md#wi-fi-credential-rules); MQTT port 1–65535) | Show validation error on form |
| 2 | Attempt STA connection to provided Wi-Fi (timeout `[tune]` 15 s) | Show "Connection failed" on portal, remain in AP |
| 3 | On STA success: store `mqtt/*`, arm MQTT client, and attempt connect (timeout `[tune]` 10 s) — same session rules as [mqtt-protocol.md](mqtt-protocol.md#session-lifecycle) | Store Wi-Fi anyway, show MQTT warning, allow retry |
| 4 | Store all config to NVDM (`wifi/*`, `mqtt/*`) | — |
| 5 | Show success page with 3 s countdown | — |
| 6 | Reboot into normal mode | — |

## Re-provisioning (pin-hole button, P0.4)

### Short press (< 1 s)

1. Enter AP mode temporarily.
2. Keep stored credentials in NVDM (not cleared).
3. AP mode timeout: `[tune]` 30 s.
4. If no client connects within timeout: retry stored Wi-Fi credentials.
5. If client connects: serve captive portal as above.

### Long press (> 7 s)

1. Clear all NVDM namespaces (wifi, mqtt, feed, display, schedule, time,
   calib, power, system).
2. Reboot.
3. Device enters first-boot AP mode.

## Security considerations

- MQTT credentials stored in NVDM flash — **not encrypted** at rest.
  No hardware secure element on MT7682. Acceptable for home-network
  threat model. `[design]`
- AP mode WPA2 with device-unique PSK preferred over open AP.
- No cloud callback, no phone-home, no telemetry.
- Captive portal serves HTTP only (no TLS on AP interface — self-signed
  cert UX is worse than plain HTTP on a local AP). `[design]`
