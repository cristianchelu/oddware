# @oddware/esphome.petlibro-plwf105

ESPHome firmware for the **PetLibro Dockstream PLWF105** water fountain.
The stock hardware already has everything worth having — an ESP32-C3, a
load cell, an HX711 — it just reports to PetLibro's cloud through their
app. Reflashing keeps the hardware and moves the smarts onto your LAN:
water level in actual millilitres, every drink logged with volume and
duration, and the front button doing more than Wi-Fi pairing. No cloud,
no account.

This is my take on
[taylorfinnell/petlibro-esphome](https://github.com/taylorfinnell/petlibro-esphome),
which did the hard part — the pin map and the flashing procedure. That
config calibrates by capturing raw ADC counts at the two fill lines; this
one calibrates the scale in grams with a tare and a known weight, which is
what makes per-drink volumes possible. The other changes are drinking
detection, water/filter reminders, and an on-device button-and-LED UX so
routine chores don't need a phone or a dashboard.

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
- Optionally, load-cell temperature from a DS18B20 soldered to the RX
  flashing pad (GPIO20, free after flashing since the UART logger is off).
  Without one the sensor simply never reports.

Somewhere for the data to go: Home Assistant works, but
[cat-health](https://github.com/cristianchelu/cat-health) pairs better —
it speaks the ESPHome API natively, no HA required, and turns per-drink
events into per-cat intake instead of a graph of bowl weights.

## The front button

Stock firmware uses the button for Wi-Fi pairing. Here it runs everything:
hold it, the LEDs announce each mode in turn, release on the one you want.

| Hold for | LEDs | Mode |
|----------|------|------|
| under 2 s | steady | nothing |
| 2–5 s | slow status pulse | consumable reset |
| over 5 s | both alternating slowly | tare & calibrate |

In consumable reset mode you have 10 seconds: a short click marks the
water changed, holding 2 seconds marks the filter changed. A bright strobe
confirms either; the mode times out back to normal on its own.

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

The default tare and coefficient are from my unit; run a calibration once
after flashing. First weigh your bowl assembly — bowl, pump, spout, pump
filter, filter cover, water tray, completely dry, no water filter — and
set the `bowl_weight` substitution. It's subtracted from the scale to get
Water Amount, and it's also the known weight the calibration expects.

1. Hold the front button until both LEDs alternate slowly (past 5 s),
   then release.
2. Lift the bowl assembly off. The firmware waits for the readings to
   settle, then tares the empty base.
3. When the LEDs switch to a fast alternate, put the dry assembly back on.
   That's the known weight; the coefficient is computed from it.

A bright strobe means the new values stuck; a fast error pulse means it
gave up (nothing removed, nothing placed, or too much wobble) and nothing
changed. Calibration survives reboots.

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
| Load Cell Temperature | sensor, °C | Only reports with a DS18B20 fitted |
| Calibration Last Performed | sensor, timestamp | Diagnostic |
| Calibration Known Weight | number, g | Diagnostic, disabled by default |
| Scale / Unfiltered Weight | sensor, g | Diagnostic; Unfiltered disabled by default |
| Operation Mode | text_sensor | Normal / Consumable Reset / Tare / Calibration |
| Activity | binary_sensor | Motion class, `delayed_off` 10 s |
| Pump Status | binary_sensor | Problem class, from the pump's fault signal |
| Vibration Detected | binary_sensor | Diagnostic; gates tare and calibration |
| Front Button | binary_sensor | Diagnostic, disabled by default |
| Pump | switch | `restore_mode: ALWAYS_ON` |
| Status LED / Error LED | light | The two stock front LEDs |
| Pet Drinking | event | Fired per qualifying drink |

## Roadmap

- [ ] Temperature compensation for the scale. The DS18B20 sits on the load
  cell and averages over two minutes precisely to watch this drift; so far
  it's observed, not corrected.
