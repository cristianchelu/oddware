# Project overview

## Target hardware

The **Xiaomi Smart Pet Food Feeder 2** (retail model XMWSQ02, cloud model
`xiaomi.feeder.iv2001`) is a single-board IoT pet feeder built around a
MediaTek MT7682 Cortex-M4F Wi-Fi SoC. It has no separate host MCU — the
application, connectivity, and peripheral drivers all run on the one chip.

Key hardware (detail in `10-hardware/`):

- **SoC:** MT7682 inside a Xiaomi MHCW05P-B module (802.11 b/g/n 2.4 GHz)
- **GPIO expander:** AW9523B (I2C, 16-pin)
- **Weighing:** CS1270 load-cell ASSP (UART)
- **Display:** TM1637 7-segment (bit-bang)
- **Motor:** single auger via SGM42507 H-bridge
- **Analog mux:** NC7SB3157 (battery/motor sense to single ADC)

## Project goals

1. **Local-only control** via MQTT — compatible with Home Assistant, Homey,
   or any broker. No cloud dependency, no phone-home.
2. **Better than stock** — the factory firmware is unreliable and poorly
   designed. We aim for correct dispense accuracy, proper jam handling,
   and robust scheduling without the proprietary cloud stack.
3. **Open source** — permissively licensed, free of proprietary
   firmware artifacts. Implementation derives exclusively from these specs
   (see [AGENTS.md](../AGENTS.md)).
4. **Safe** — motor/ADC supervision, stall detection, brick-recovery path.

## Naming

- **Repository path:** `xiaomi.feeder.iv2001` — the device's cloud model ID,
  used descriptively to identify compatible hardware.
- **Project codename:** none needed; the repo path is the identifier.
- **MQTT namespace:** `petfeeder/<device_id>/` — fully neutral, no brand strings.
- **Firmware binary:** no Mi/MIoT/Xiaomi strings compiled into shipped code.

## Non-goals

- 1:1 behavioral compatibility with the factory firmware.
- MIoT / Mi Home / Xiaomi cloud integration.
- Support for hardware other than the XMWSQ02 / IV2001 board revision.


## Scope of this spec tree

The spec tree is organized in three tiers:

- **Tier 1 (`10-hardware/`)** — hardware invariants: the physical PCB,
  its components, pins, and electrical constraints.
- **Tier 2 (`20-stories/`)** — user-facing goals: what the device should
  do, written as stories or epics.
- **Tier 3 (`30-processes/`)** — process descriptions: detailed engineering
  mechanisms with testable assertions and `[tune]` parameters.

These tiers define **what** the firmware must do and **how** at a process
level — but not the software architecture. The firmware's task model, module
boundaries, and priorities are intentionally deferred to a fresh derivation
from these specs, free from factory-firmware influence.

Behavioral changes are tracked as git diffs on spec files. There is no
separate changelog.
