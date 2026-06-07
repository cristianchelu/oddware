# Power architecture

## Inputs


| Source                 | Voltage               | Current                   | Provenance  |
| ---------------------- | --------------------- | ------------------------- | ----------- |
| Barrel adapter (mains) | 5.9 V DC              | 1 A rated                 | `[product]` |
| Battery backup         | 4x AA alkaline (~6 V) | emergency dispensing only | `[product]` |


## Mains detection

- **AW9523B P1.1** reads mains/barrel present as a digital level `[probe]`
- IRQ on insertion/removal triggers power-state update

## Battery / motor ADC path

A single ADC channel on **GPIO17** is shared via the **NC7SB3157** SPDT mux:


| Mux select (P1.7) | Path     | Purpose                            |
| ----------------- | -------- | ---------------------------------- |
| High              | B1 → COM | Battery voltage sense              |
| Low               | B0 → COM | Motor load sense (during dispense) |


ADC reference: 2500 mV full-scale, 12-bit `[ds:MT7682]`

## Rails (incomplete)


| Rail         | Voltage                    | Consumers                     | Provenance              |
| ------------ | -------------------------- | ----------------------------- | ----------------------- |
| Module VCC   | 3.3 V                      | MT7682, AW9523B, CS1270 logic | `[ds:MT7682]` `[probe]` |
| Motor supply | ~5–6 V (barrel or battery) | SGM42507 VM                   | `[probe]`               |
| Display      | switched via P0.5          | TM1637 + LEDs                 | `[probe]`               |


## Gaps

- Regulator topology (LDO vs buck) between barrel/battery and 3.3 V unknown.
- Battery-to-motor path not fully traced.
- Power-on sequencing not characterized.

