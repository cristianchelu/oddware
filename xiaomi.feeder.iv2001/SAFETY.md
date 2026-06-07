# Safety

## Read before flashing

This firmware is experimental. Flashing it onto your device carries real risks:

1. **Bricking.** A failed flash can render the device unbootable. Recovery requires
  UART access to the module's boot ROM pads — you must be comfortable soldering
   fine-pitch test points.
2. **Electrical hazard.** The device uses a 5.9 V barrel adapter and a motor driver.
  Incorrect firmware can leave outputs in undefined states. Never leave a freshly
   flashed device unattended until you have verified safe idle behavior.
3. **Pet safety.** This is a pet feeder. Firmware bugs can cause over-dispensing,
  motor stalls with food jammed, or complete failure to dispense. Always maintain
   a backup feeding plan for your animals during development and early adoption.
4. **No warranty.** Factory warranty is void once you flash third-party firmware.
  There is no guarantee of any kind — use at your own risk.

## Recovery

The MT7682 boot ROM is always available via UART0 (module pads GPIO21/GPIO22).
Even with a fully erased flash, the MediaTek IoT Flash Tool can reprogram the
entire chip from scratch. MAC is in efuse — you cannot lose device identity.

Keep a known-good full-flash image available for emergency restore.

## Do not sell pre-flashed devices

Selling devices with this firmware pre-installed creates product-liability exposure
and potential trademark issues. This project is for personal/hobbyist use on
hardware you own.

## Motor and mechanical safety

- The auger motor is physically capable of jamming hard enough to strip gears
or stall with high current draw. Always implement stall detection and timeout.
- The motor driver (SGM42507) has an EN/FAULT line. Monitor it.
- Never run the motor continuously without index-pulse or load-ADC supervision.

## Electrical notes

- The module operates at 3.3 V logic. The motor driver, load cell ASSP, and GPIO
expander share the same single PCB. Do not apply external voltages to I/O lines
without confirming levels against the component datasheets.
- The barrel input is nominally 5.9 V / 1 A. Battery backup (4x AA) is ~6 V.
The power path includes a mux; understand it before modifying power-related code.

