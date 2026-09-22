# @oddware/esphome.petkit-eversweet-3-pro

A replacement base for the **PetKit Eversweet 3 Pro UVC** water fountain that
weighs the whole fountain and logs every drink to Home Assistant. The fountain
itself is untouched — it sits on the new base and gets its power the same way
it always did, inductively, through its own coil.

| In service | A drink, logged |
|:---:|:---:|
| ![The fountain running on the base](docs/build-in-service.jpg) | ![cat-health recording Jazz drinking](docs/cat-health-water-intake.png) |

## Why

The Eversweet 3 Pro is a genuinely good fountain. It strips down in seconds,
there's nothing awkward to scrub, and the UVC and filters keep it pleasant to
live with. But it has no scale, so it can't tell you how much your cat
actually drank. The app offers runtime counters and filter reminders
instead, and even those go through PetKit's cloud and an account.

So the fountain stays as-is and the base under it gets replaced. Four load
cells weigh the entire fountain continuously; every drink is logged with
volume and duration, straight into Home Assistant over the local API. No
cloud, no account, and the cleaning routine doesn't change.

Ironically, the stock base already has most of this on its one board — a
current-sense ADC, a switching transistor, a BLE chip. It just does it all
for PetKit. This base keeps only the dumb part, the coil and its driver, and
redoes the smarts with parts an ESP32 can run: an INA219 for the current
sensing, a MOSFET for the switching, and a scale, which the original never
had. The coil comes out of the stock base on double-sided tape and
a connector, no soldering, so a fresh sticky pad puts everything back to
stock if you ever want out.

## What it does

- Water volume and fill level (mL and %) from 4 load cells + HX711
- Drinking events with volume and duration, plus an `Activity` binary sensor
- Pump on/off as a Home Assistant switch, and pump power draw from the INA219
- UVC lamp state, sensed from coil power, with a lamp-fault flag when the
  pump's schedule says an edge is overdue
- Foreign-object detection: coil power cut within half a second, probed
  again every 5 s
- Water-change and filter-change reminders with configurable intervals and
  reset buttons
- RGB status LED through a light guide: colour = water level, breathing =
  consumable overdue, blue = Wi-Fi down, purple strobe = tare/calibration,
  red 3 Hz strobe = foreign object on the coil
- Runtime tare and known-weight calibration from Home Assistant, persisted
  across reboots

## What you'll need

- The fountain, and its stock base as the coil donor
- A 3D printer with a ~200 mm bed, and PLA
- The electronics — ESP32-C3 SuperMini, HX711 + 4 half-bridge load cells,
  INA219, LR7843 — plus connectors, an O-ring, an acrylic rod, and 22 M3
  screws; full list in [hardware.md](hardware.md#parts-to-buy)
- A donor **Tefal Optiss kitchen scale** for its
  [load-cell feet](hardware.md#about-those-load-cell-feet)
- Soldering iron, XH2.54/Dupont crimper, multimeter, hot glue gun
- Somewhere for the data to go: Home Assistant, or
  [cat-health](https://github.com/cristianchelu/cat-health), which speaks
  the ESPHome protocol natively — no HA required

## Telling the cats apart

A scale can't tell which cat is drinking, and in a multi-cat house per-cat
intake is the number that matters. That part is solved outside this project:
point a camera at the fountain, trigger a snapshot off the `Pet Drinking`
event, and have a vision model ([OpenRouter](https://openrouter.ai) or
similar) identify the cat.

Pairs well with
[cristianchelu/cat-health](https://github.com/cristianchelu/cat-health),
the nexus of everything cat-related around here — feed it the per-cat
drinks and this goes from a fountain that logs weights to actual health
monitoring.

## Home Assistant entities

| Entity | Type | Notes |
|--------|------|-------|
| Water Amount | sensor, mL | Stable weight minus `bowl_weight` |
| Water Level | sensor, % | Scaled between `water_mark_min` and `water_mark_max` |
| Water Rate of Change | sensor, mL/min | Diagnostic; drives drinking detection |
| Coil Power | sensor, W | INA219 on the transmitter supply, 5 s average |
| Last drink amount / duration | sensor | Published on each qualifying drink event |
| Water / Filter Time Remaining | sensor, days | |
| Water / Filter Change Due | sensor, timestamp | |
| Calibration Last Performed | sensor, timestamp | Diagnostic |
| Scale / Unfiltered Weight | sensor, g | Diagnostic, disabled by default |
| Scale Tare Offset / Coefficient | sensor | Diagnostic, disabled by default |
| WiFi Signal | sensor, dBm | Diagnostic; median of four samples, once a minute |
| Activity | binary_sensor | Motion class, `delayed_off` 10 s |
| UVC On | binary_sensor | Sensed: lit at power-up, then follows the edges in Coil Power |
| UVC Lamp Fault | binary_sensor | Problem class; the schedule expected an edge and none showed |
| Foreign Object | binary_sensor | Problem class; coil held off, probed every 5 s |
| Pump Missing | binary_sensor | Problem class; coil draw at the empty-coil level |
| Coil Energized | binary_sensor | Diagnostic; the actual GPIO state |
| Vibration Detected | binary_sensor | Diagnostic; gates tare and calibration |
| Operation Mode | text_sensor | Normal / Tare / Calibration |
| Pump | switch | What you want; the coil follows unless a foreign object holds it off. `restore_mode: ALWAYS_ON` |
| Status LED | light | Single WS2812 |
| Status LED Brightness | select | Low / Medium / High |
| Calibration Known Weight | number, g | |
| Water / Filter Change Interval | number, days | |
| Water Changed / Filter Changed | button | Resets the corresponding timer |
| Tare Scale / Calibrate Scale / Cancel Calibration | button | |
| Pet Drinking | event | Fired per qualifying drink |

## Flash

1. Copy [`secrets.yaml.example`](secrets.yaml.example) to `secrets.yaml`
   (gitignored) and set Wi-Fi, API, and OTA credentials.
2. Validate and flash [`petkitwaterfountain.yaml`](petkitwaterfountain.yaml):

   ```bash
   esphome config petkitwaterfountain.yaml
   ```

   ```bash
   esphome run petkitwaterfountain.yaml
   ```

The ESP32-C3 SuperMini builds as `esp32-c3-devkitm-1` on ESP-IDF. First flash
over USB, OTA after that.

## Calibrate

> **Put it on a flat, hard surface first.** A four-footed scale only reads
> right when all four feet carry load. On a rug or a rocking tile you get
> drifting baselines and phantom drinking events, and no amount of
> calibration will fix it.

The scale weighs the whole fountain, so "tare" means *with the fountain
lifted off the base*, not with an empty reservoir.

1. Lift the fountain off. Press **Tare Scale** — the LED strobes purple
   while the firmware waits for vibration to settle, then the reading is
   stored as the tare offset.
2. For a full calibration, set **Calibration Known Weight**, press
   **Calibrate Scale** with the base empty, then place the known mass when
   the strobing speeds up. A green flash means the new coefficient stuck.
3. Set the `bowl_weight` substitution to the dry weight of the fountain —
   body, pump, spout, filter, empty reservoir. It's subtracted to get Water
   Amount, so getting it wrong shifts every reading.
4. Set `water_mark_min` / `water_mark_max` to the mL readings you want shown
   as 0 % and 100 %.

Calibration survives reboots. To restore a known-good pair without redoing
the physical flow, call the `set_calibration` API action with `tare` (int)
and `coefficient` (float).

The default `scale_coefficient` is negative (`-282.8`). The sign only
depends on which way round the load-cell bridge was wired; the firmware
handles either.

## UVC and foreign objects

The pump's own controller runs the UVC lamp, and the coil only carries
power, so both features come from the INA219 alone. The lamp shows up as a
~0.1 W step on the pump's draw. Metal on the coil shows up as excess draw.

| On the coil | Coil Power |
|-------------|------------|
| Nothing | 0.27 W |
| Pump, lamp off | 0.40–0.70 W, depending on how it seats on the spout |
| Pump, lamp on | 0.10 W more |
| A foreign object | over 1.0 W |

`UVC On` is what the coil sees. The pump starts lit every time it gets
power, and after that every step of 0.05 W or more in Coil Power, up or
down, flips it. The detector compares the last 30 s against the 30 s
before, so it reports an edge about 15 s late.

The pump's own schedule, nominally:

| Phase | Lamp |
|-------|------|
| First 3 h | on |
| Next 3 h | off |
| Then, repeating | 1 h on, 3 h off |

The firmware replays that clock only to know when an edge is due. Every
edge it sees re-anchors the clock, so the pump's real phase lengths (the
off-phase measures closer to 3 h 05) never add up:

- Falling edge — the clock is pinned exactly: every off-phase is 3 h, then
  1 h on.
- Rising edge — taken as the start of a 1 h phase.
- Edge due, none seen for 15 min — `UVC Lamp Fault`. The next edge clears
  it.
- Coil Power under 0.33 W for 10 s — `Pump Missing`. Back over 0.38 W for
  10 s — clears, and the clock restarts.

Foreign objects:

- Two raw samples over 1.0 W, 200 ms apart — coil off, `Foreign Object` on,
  LED strobes red at 3 Hz. About half a second from contact to cut.
- Every 5 s — the coil comes back for 1.5 s. Still over — off again.
  Clear — normal running, and the UVC clock restarts.
- `Pump` switched off — the retry stops and the flag clears.

The threshold sits 0.2 W above the highest legitimate draw, so a small
screw or an off-centre coin adds too little to trip it. It catches keys and
cutlery, not a paperclip.

## Build

Print the parts, order the rest, and assemble per [hardware.md](hardware.md).

Two of the steps involve glue — the coil shell bonds into the outer shell,
and the USB-C pass-through gets sealed with hot melt — so dry-fit and
power-test everything first.

## Roadmap

- [ ] A capacitive button on the base to mark water/filter changed without
  reaching for Home Assistant.
- [ ] A foreign-object threshold relative to the learned seated baseline,
  to catch smaller objects than the fixed 1.0 W does.
