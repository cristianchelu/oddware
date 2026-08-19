# @oddware/esphome.petlibro-plwf105

ESPHome firmware for the **PetLibro Dockstream PLWF105** water fountain.
The stock hardware already has everything worth having — an ESP32-C3, a
load cell, an HX711 — it just reports to PetLibro's cloud through their
app. Reflashing keeps the hardware and moves the smarts onto your LAN:
water level in actual millilitres, every drink logged with volume and
duration, and the front button doing more than Wi-Fi pairing. No cloud,
no account.

This is my take on
[taylorfinnell/petlibro-esphome](https://github.com/taylorfinnell/petlibro-esphome)
— same pin map, different firmware: scale calibration by tare and known
weight instead of two fill-line captures, per-drink events with volume
and duration, water/filter reminders, and a button-and-LED UX for the
routine chores.

## What it does

- Water amount (mL) and fill level (%) from the built-in scale
- Drinking events with volume and duration, plus an `Activity` binary
  sensor and a `Pet Drinking` event
- Water-change and filter-change reminders with configurable intervals,
  reset from Home Assistant or the front button
- Pump as a switch; the pump's fault signal as a `problem` sensor that
  drives the error LED
- Tare and known-weight calibration from the front button, vibration-gated,
  persisted across reboots

Somewhere for the data to go: Home Assistant works, but
[cat-health](https://github.com/cristianchelu/cat-health) pairs better —
it speaks the ESPHome API natively, no HA required, and turns per-drink
events into per-cat intake instead of a graph of bowl weights.

## The front button

Stock firmware uses the button for Wi-Fi pairing. This firmware runs
everything through it. Hold the button; the LEDs show each mode in turn.
Release to enter the mode shown.

| Hold for | LEDs | Mode |
|----------|------|------|
| under 2 s | steady | nothing |
| 2–5 s | slow status pulse | consumable reset |
| over 5 s | both alternating slowly | tare & calibrate |

In consumable reset mode:

- Short click — water changed.
- Hold 2 s — filter changed.
- Bright strobe — reset stored.
- No input for 10 s — back to normal.

## Flash

Opening the base exposes `GND`/`TX`/`RX` pin holes for a USB-serial
adapter and a pair of pads (`B` + `G`) that put the ESP32-C3 in download
mode when shorted while power is plugged in. Follow the
[upstream instructions](https://github.com/taylorfinnell/petlibro-esphome)
for the full procedure, and back up the stock firmware first as described
there — PetLibro doesn't hand out copies.

1. Copy [`secrets.yaml.example`](secrets.yaml.example) to `secrets.yaml`
   (gitignored) and set Wi-Fi, API, and OTA credentials.
2. Validate and flash [`water-fountain.yaml`](water-fountain.yaml):

   ```bash
   esphome config water-fountain.yaml
   ```

   ```bash
   esphome run water-fountain.yaml
   ```

First flash over serial, OTA after that.

## Calibrate

The known weight is the dry bowl assembly: bowl, pump, spout, pump
filter, filter cover, water tray. No water filter, no water. Every
fountain ships with one; it is the `bowl_weight` substitution (525 g),
also subtracted from the scale to get Water Amount.

The default tare and coefficient are the factory fallback values from the
stock firmware image, calibrated against a flat 500 g reference — close,
not exact. Run one calibration after flashing.

1. Hold the front button past 5 s. Release when both LEDs alternate
   slowly.
2. Lift the bowl assembly off. The firmware waits for the readings to
   settle, then tares the empty base.
3. Wait for the LEDs to alternate fast. Put the dry assembly back on.
   The coefficient is computed from it.

Results:

- Bright strobe — new values stored. They survive reboots.
- Fast error pulse — failed: nothing removed, nothing placed, or too
  much wobble. Nothing changed.

`water_mark_min` and `water_mark_max` set which mL readings show as 0 %
and 100 % — defaults match the bowl's 500 mL MIN line and 2300 mL brim.

## Drinking detection

Water Amount dropping faster than 15 mL/min turns `Activity` on; it stays
on until 10 s after the last activity. The drink volume is the level from
5 s before the start minus the level at the end, so the beginning of the
drink isn't clipped. Events under 1 mL or 1 s are discarded; qualifying
ones publish the last-drink sensors and fire `Pet Drinking`. All
thresholds are substitutions.

## Home Assistant entities

| Entity | Type | Notes |
|--------|------|-------|
| Water Amount | sensor, mL | Stable weight minus `bowl_weight` |
| Water Level | sensor, % | Scaled between `water_mark_min` and `water_mark_max` |
| Water Rate of Change | sensor, mL/min | Diagnostic; drives drinking detection |
| Last drink amount / duration | sensor | Published per qualifying drink |
| Water / Filter Time Remaining | sensor, days | |
| Water / Filter Change Due | sensor, timestamp | |
| Water / Filter Change Interval | number, days | |
| Water Changed / Filter Changed | button | Resets the corresponding timer |
| Calibration Last Performed | sensor, timestamp | Diagnostic |
| Calibration Known Weight | number, g | Diagnostic, disabled by default |
| Scale / Unfiltered Weight | sensor, g | Diagnostic; Unfiltered disabled by default |
| WiFi Signal | sensor, dBm | Diagnostic; median of four samples, once a minute |
| Operation Mode | text_sensor | Normal / Consumable Reset / Tare / Calibration |
| Activity | binary_sensor | Motion class, `delayed_off` 10 s |
| Pump Status | binary_sensor | Problem class, from the pump's fault signal |
| Vibration Detected | binary_sensor | Diagnostic; gates tare and calibration |
| Front Button | binary_sensor | Diagnostic, disabled by default |
| Pump | switch | `restore_mode: ALWAYS_ON` |
| Status LED / Error LED | light | The two stock front LEDs |
| Pet Drinking | event | Fired per qualifying drink |
