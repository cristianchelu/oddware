# Provisioning flow

serves:
  - ../20-stories/provisioning.md

## AP mode entry

AP and HTTP server bring-up run from an application FreeRTOS task after
`vTaskStartScheduler()` — LinkIt Wi-Fi configuration APIs require the
in-band task to be running. The MQTT client task does not start when no
Wi-Fi credentials are stored (frees heap for AP + DHCP + httpd). HTTP
starts after a short settle delay once AP mode is up.

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

Minimal HTTP server at 192.168.4.1 serving a single-page form. No HTTP
authentication — the open AP is the only access control during setup.

| Field | Type | Required | Default |
|-------|------|----------|---------|
| Nearby Wi-Fi networks | select (`wifi_ssid_pick`) | no | — (empty = manual entry) |
| Wi-Fi SSID | text (`wifi_ssid`) | yes | — (manual entry overrides pick) |
| Wi-Fi password | password | no | (empty = open network; validation: [uart-console.md](uart-console.md#wi-fi-credential-rules)) |
| MQTT broker host | text | yes | — |
| MQTT broker port | number | no | 1883 (empty field = default; non-empty must be integer 1–65535) |
| MQTT username | text | no | (empty = anonymous) |
| MQTT password | password | no | (empty) |
| Device ID | text | no | MAC-derived |

Form submission via HTTP POST. No JavaScript dependency — should work in
captive portal webviews on all platforms.

STA test-connect stops the AP while the phone is still on the POST request.
The handset usually roams back to its home Wi-Fi before the device can send
the POST response, so errors are **not** shown inline on the original
request. On Wi-Fi or MQTT failure the device stashes the user-visible
message and repopulated field values in RAM; the next GET `/provision.cgi`
after the user reconnects to `PetFeeder-XXXX` shows the alert and filled
form. The stash survives captive-portal probe GETs (peek, not consume on
GET); it clears on the next form POST or successful submit.

### HTTP interface

| Method | Path | Response |
|--------|------|----------|
| GET | `/` or `/index.html` | Meta refresh to `/provision.cgi` plus fallback link (no JavaScript) |
| GET | `/provision.cgi` | HTML form with cached scan list (empty fields, or repopulated after error) |
| GET | `/provision.cgi?rescan=1` | Re-scan nearby APs (partial scan in AP mode), then same form |
| POST | `/provision.cgi` | Save + connect flow result page |

POST body is `application/x-www-form-urlencoded`. Field names match the
table above:

| Form field | Maps to |
|------------|---------|
| `wifi_ssid_pick` | UI only — copied to `wifi_ssid` when the text field is empty |
| `wifi_ssid` | NVDM `wifi/ssid` |
| `wifi_pass` | NVDM `wifi/pass` (empty = open network) |
| `mqtt_host` | NVDM `mqtt/host` |
| `mqtt_port` | NVDM `mqtt/port` (empty = 1883) |
| `mqtt_user` | NVDM `mqtt/user` |
| `mqtt_pass` | NVDM `mqtt/pass` |
| `device_id` | NVDM `mqtt/device_id` (empty = MAC-derived) |

User-visible error strings on the form page:

| Condition | Message |
|-----------|---------|
| Validation failure | `Please fix the highlighted fields.` |
| Wi-Fi association or DHCP timeout (`[tune]` 15 s) | `Could not connect to "<ssid>". Check the network name and password, then try again.` — form repopulated (including password fields), `role="alert"` on the message |
| Wi-Fi OK, MQTT connect timeout (`[tune]` 10 s) | `Could not connect to the MQTT broker. Check the host, port, and credentials, then try again.` — form repopulated, `role="alert"` on the message |
| Probes OK, NVDM save failed | `Could not save configuration. Try again.` — form repopulated, `role="alert"` on the message |
| Success (before reboot) | `Setup complete. Rebooting in 3 seconds…` |

## Wi-Fi scan list

While AP provisioning is active, the device runs a **partial** Wi-Fi scan
(SDK `wifi_connection_start_scan` mode 1 — safe with AP up). Results are
cached and shown in a `<select>` sorted by RSSI. The device's own AP SSID
(`PetFeeder-XXXX`) is omitted. Hidden SSIDs are skipped.

| When | Action |
|------|--------|
| AP + HTTP bring-up | Scan once before httpd starts (`[tune]` 10 s timeout) |
| GET `?rescan=1` | Refresh cache |

If the scan fails or finds no networks, the form still works via manual SSID
entry. A “Refresh network list” link points to `?rescan=1`.

## SDK STA profile abort

Provisioning test-connect, factory reset, and AP entry call
`wifi_adapter_clear_sdk_sta_profile()` to stop the LinkIt STA radio from
retrying a rejected association. Order:

| Step | SDK action |
|------|------------|
| 1 | `wifi_connection_stop_scan()` |
| 2 | `wifi_connection_disconnect_ap()` |
| 3 | Clear NVDM `STA/SsidLen`, `STA/Ssid`, `STA/WpaPskLen`, `STA/WpaPsk` |
| 4 | `wifi_config_reload_setting()` |

App NVDM `wifi/*` is untouched — only the SDK HAL profile is cleared.

## Provisioning STA test-connect

`provision_wifi_try_connect()` (used by step 2 of save + connect):

| Phase | Action |
|-------|--------|
| Enter STA test | Stop HTTP, leave AP (`stop_ap`), `display_wifi_indicator_connecting()`, `connect` + wait up to `[tune]` 15 s for IP |
| Success | Return true — caller saves credentials |
| Failure | `wifi_adapter_clear_sdk_sta_profile()`, 200 ms settle, `start_ap`, `display_wifi_indicator_ap_mode()`, restart HTTP on port 80 — **no** scan refresh |

Restore runs even when `connect` returns an error or `stop_ap` fails (best effort).
AP/HTTP restore after a failed POST is scheduled **after** the CGI response
is sent. The LinkIt httpd task cannot stop or restart itself while a CGI
handler is still running (`httpd_start()` during `HTTPD_STATUS_STOPPING`
does not queue a restart). Wi-Fi test-connect does not call `httpd_stop()`
from the CGI path; a one-shot FreeRTOS timer (`[tune]` 250 ms) restarts HTTP
(or AP + HTTP) from the timer service task once the current request completes.
Restore must not run inline in the CGI path — even as a fallback.

## Save + connect flow

On form submit:

| Step | Action | On failure |
|------|--------|------------|
| 1 | Validate inputs (Wi-Fi SSID/password per [uart-console.md](uart-console.md#wi-fi-credential-rules); MQTT port 1–65535) | Show validation error on form |
| 2 | Attempt STA connection to provided Wi-Fi (timeout `[tune]` 15 s) | Abort STA (`disconnect` + clear SDK `STA/*` + `reload_setting`), restore AP + HTTP, show error on portal — **no** rescan on this path |
| 3 | On STA success: probe MQTT broker with submitted credentials (timeout `[tune]` 10 s) — same session rules as [mqtt-protocol.md](mqtt-protocol.md#session-lifecycle) | Abort STA, restore AP + HTTP, show MQTT error on portal — **no** NVDM write |
| 4 | On Wi-Fi and MQTT success: store `wifi/*` and `mqtt/*` to app NVDM | Erase any partial `wifi`/`mqtt` keys written during this submit, restore AP + HTTP, show save error — **no** credentials left stored |
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
