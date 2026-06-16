# Provisioning

## Overview

The feeder needs Wi-Fi and MQTT broker details before it can operate.
Setup happens through a built-in captive portal — no app or cloud
account required.

## First-boot flow

1. On first power-on (or after factory reset) the feeder starts a
   Wi-Fi access point: **PetFeeder-XXXX** (XXXX = last 4 of MAC). The
   panel Wi-Fi pictograph blinks rapidly so the user can tell the feeder
   is in setup mode without opening the phone (see [display.md](display.md)
   § Wi-Fi indicator).
2. The user connects to the AP and opens a captive-portal web page.
3. The portal presents a simple form:
   - Wi-Fi network name and password.
   - MQTT broker host and port.
   - MQTT username and password (optional).
   - Device ID (optional; defaults to a MAC-derived value).
4. On submit the feeder attempts to connect. While validating Wi-Fi
   credentials the pictograph switches to the slower connecting blink;
   on failure it returns to the setup blink when AP mode is restored.
   On success it saves the config, reboots into normal mode, and
   publishes its online state to the broker.
5. On failure it remains in AP mode and shows the error so the user
   can retry.

## Re-provisioning

- A short press of the pin-hole reset button temporarily re-enters AP
  mode (times out after ~30 s, then retries stored credentials).
- Useful for changing the Wi-Fi network or MQTT broker without a full
  reset.

## Factory reset

- A long press of the pin-hole reset button (~7 s) erases all stored
  configuration and reboots into the first-boot provisioning flow.

## Security

- Setup AP is open (no password).
- MQTT credentials are stored in flash (unencrypted — acceptable for a
  home-network threat model).
- No cloud callback, no phone-home, no telemetry.
