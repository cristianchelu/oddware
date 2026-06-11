# NC7SB3157 — SPDT analog switch

## Summary

Ultra-low-voltage single-pole double-throw analog switch. Routes either the
battery voltage sense or the motor load sense to the SoC's single ADC input.

## Interface

| Pin | Connected to | Source |
|-----|--------------|--------|
| COM (pin 4) | MT7682 GPIO17 (ADC) | `[probe]` |
| B0 (pin 3) | Motor current sense (1 Ω shunt → ADC) | `[probe]` |
| B1 (pin 1) | Battery voltage divider | `[probe]` |
| S (pin 6) | AW9523B P1.7 (select) | `[probe]` |

## Select logic

| S (P1.7) | Active path | Purpose |
|-----------|-------------|---------|
| High | B1 → COM | Battery voltage measurement |
| Low | B0 → COM | Motor load measurement |

Source: `[ds:NC7SB3157 §3]`

## Key specs

| Parameter | Value | Source |
|-----------|-------|--------|
| Supply | 1.65–5.5 V | `[ds:NC7SB3157 §4]` |
| Ron | ~5 Ω typical | `[ds:NC7SB3157 §4]` |
| Bandwidth | 300 MHz | `[ds:NC7SB3157 §4]` |
| Break-before-make | yes | `[ds:NC7SB3157 §4]` |

## Application notes

- Only one measurement at a time: switch mux, wait for settling `[tune]`,
  then sample ADC.
- During dispense, mux stays on motor (S=low) for stall detection. Battery
  reads happen when motor is idle.
- ADC conversion: raw × 2500 / 4095 = millivolts `[ds:MT7682]`
- Battery voltage divided before B1; nominal ~11:1 `[design]`. Firmware default
  11/1; per-device trim via NVDM (`battery-monitoring.md`). Typical measured
  ratio ~10.6–10.7:1 `[probe]`.
- Firmware mux select constants: `board_gpio_iv2001.h` (`BOARD_GPIO_ADC_MUX_*`).
  Read API: `adc_port` ([ports.md](../../40-architecture/ports.md)).
