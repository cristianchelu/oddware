# Hardware and enclosure

You’re building a small scale with a tower beside it. The **base** and
**platform** are the weighing platform — load cell in the middle, your own bowl
on top. The **tower shell** hangs the camera over the bowl and hides the
boards. The **tower back cover** is how you get inside and where USB power
comes in.

## CAD and print files

| Property | Value |
|----------|-------|
| Name | Water Bowl Monitor v7 |
| Tool | Autodesk Fusion 360 |
| Archive | `hardware/models/pet-bowl-monitor.f3z` |

Print files live in `hardware/models/` — one `*.3mf` per part below, plus the
Fusion archive above.

**Filament:** plain PLA. Fusion’s material preset says ABS; ignore that.

## Printable parts

| Part | What it does |
|------|----------------|
| **Platform** | Top of the scale. Your bowl sits here. |
| **Base** | Bottom of the scale. The load cell and platform mount to this; the tower bolts on here too. |
| **Tower shell** | Vertical enclosure. Holds the ESP32-CAM, camera module, IR LED, and HX711. |
| **Tower back cover** | Closes the back of the tower. The USB-C breakout mounts here. Two self-tapping screws (below) join the cover to the **base** and **tower shell** — the only fasteners in the build. |
| **Load cell clip** | Presses the load cell onto the base. No screw; clip retention only. |

The design aimed for no screws. The load cell still needs the clip, and the
back cover needs two anyway. Everything else clips or presses in.

## Parts to buy

Check that substitutes fit the printed parts before you order.

### Electronics

| Role | Part | Notes | Supplier |
|------|------|-------|----------|
| Compute | ESP32-CAM (no camera module) | Order board only; fit OV2640 separately. Desolder header pins before assembly. | [AliExpress](https://www.aliexpress.com/item/1005006137530316.html) |
| Camera | OV2640, 21 mm, 160° FOV | Must be modded no-IR-cut | [AliExpress](https://www.aliexpress.com/item/1005007291555550.html) |
| IR lighting | 940 nm IR LED, 1 W | Desolder header pins before assembly. Firmware caps PWM at 30 % (`ir_led_max_brightness: 0.30` in `pet-bowl-monitor.yaml`) — do not raise without checking heat. | [AliExpress](https://www.aliexpress.com/item/1005009665289871.html) |
| Scale | 1 kg load cell + HX711 | HX711 PCB **with two index holes**. Load cell ~80×80 mm, 2×4 mm + 2×5 mm holes | [AliExpress](https://www.aliexpress.com/item/1005001537354199.html) |
| Power input | USB-C breakout (GroundStudio, 21×17.5 mm) | Any equivalent that fits the tower back cover | [ArduShop](https://ardushop.ro/ro/groundstudio/531-modul-usb-c-groundstudio-6427854006202.html) |

### Fasteners and seals

| Role | Part | Notes |
|------|------|-------|
| Screws | 2.9×9.5 mm countersunk, self-tapping | 3×10 mm also works. Two screws total — tower back cover to base and tower shell |
| O-rings | NBR70 Shore A assortment (12 sizes) | Camera lens seal 11.0×7.2×1.9 mm. Neighboring sizes seal the USB-C connector |

### Consumables

| Role | Part | Notes |
|------|------|-------|
| Filament | PLA | Enclosure prints |

## Before you assemble

**Desolder the through-hole pins** on the ESP32-CAM and the IR LED module.
The printed pockets are sized for flush boards; leaving the headers on will
block fit.

## Load cell

The cell sits in the **base**; the **clip** snaps over it. Locating pins are
integral to the base and platform — if one breaks, reprint that part.

## Assembly

1. Clip the load cell to the **base**; wire it to the HX711.
2. Fit the HX711, ESP32-CAM, camera module, and IR module into the **tower
   shell**; wire per [Wiring](#wiring) below.
3. Seat the **platform** on the load cell.
4. Mount the USB-C breakout on the **tower back cover**; route the cable. Use
   an O-ring from the assortment above to seal the connector opening.
5. Close with the **tower back cover** — two screws into the base and tower
   shell.
6. Put your bowl on the **platform** and [calibrate](README.md#calibrate).

## Wiring

Solder only these connections to the ESP32-CAM:

| Wire | GPIO |
|------|------|
| HX711 DOUT | GPIO15 |
| HX711 SCK | GPIO13 |
| IR LED IN | GPIO3 |
| Power | Two wire pairs from USB-C breakout: one to ESP32-CAM 5 V / GND, one to IR board 5 V / GND |

Power from the USB-C breakout with two separate 5 V / GND wire pairs — one to the
ESP32-CAM and one to the IR board. Tie all grounds together.
