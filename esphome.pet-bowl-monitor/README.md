# @oddware/esphome.pet-bowl-monitor

ESPHome firmware and hardware for a **cat water bowl monitor**. A load cell
under the bowl tracks water volume and drinking events; an ESP32-CAM with IR
lighting looks down into the bowl. Reports to Home Assistant over the native
API — local-only, no cloud.

![Assembled pet bowl monitor](docs/esphome-bowl-monitor-assembled.png)

## What it does

- Water weight and fill level (mL and %) from a 1 kg load cell + HX711
- Drinking events with volume and duration
- Camera stream with 940 nm IR during activity; grayscale in low light
- White-light alerts for empty bowl, missing bowl, and overdue water change
- Water-change detection when you lift and replace the bowl
- Home Assistant sensors, binary sensors, buttons, events, and camera

## Flash

1. Copy [`secrets.yaml.example`](secrets.yaml.example) to `secrets.yaml`
   (gitignored) and set Wi-Fi, API, and OTA credentials.
2. Validate and flash [`pet-bowl-monitor.yaml`](pet-bowl-monitor.yaml):

   ```bash
   esphome config pet-bowl-monitor.yaml
   esphome run pet-bowl-monitor.yaml
   ```

## Calibrate

After the device is on Wi-Fi and in Home Assistant:

1. **Set Zero** — empty platform, no bowl.
2. **Calibrate with Known Weight** — place a known mass on the platform.
3. Set **Bowl Weight** and **Bowl Capacity** to match your bowl.

## Build

Print the enclosure, order parts, and assemble per [hardware.md](hardware.md).
