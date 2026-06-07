# Specification documents

This directory contains the hardware specifications, behavioral goals, and
process descriptions for the open-source pet feeder firmware. Implementation
(`firmware/`) is written exclusively from these specs — never from
disassembly or proprietary sources.

## Three-tier model

| Tier | Path | What it contains |
|------|------|------------------|
| 1 | [10-hardware/](10-hardware/) | Hardware invariants — the physical PCB, its components, pins, and constraints |
| 2 | [20-stories/](20-stories/) | User-facing goals — what the device should do, written as stories/epics |
| 3 | [30-processes/](30-processes/) | Process descriptions — detailed mechanisms, testable assertions, `[tune]` values |

## Reading order

1. **[00-overview.md](00-overview.md)** — goals, constraints, scope.
2. **Tier 1 (hardware)** — immovable constraints from the PCB.
3. **Tier 2 (stories)** — what the user wants the device to do.
4. **Tier 3 (processes)** — how to achieve each goal in engineering detail.

Tier 3 files include a `serves:` field (immediately after the title) linking
back to the Tier 2 stories they implement. This is many-to-many — a single
process may serve multiple stories.

## Provenance legend

Every non-trivial assertion must carry a provenance tag:

| Tag | Meaning |
|-----|---------|
| `[ds:<part> §<section>]` | Datasheet reference |
| `[probe]` | Verified by physical measurement |
| `[bootlog]` | Observed in on-the-wire UART output |
| `[product]` | From retail product page / manual / FCC filing |
| `[design]` | Our own engineering choice |
| `[tune]` | To be determined during bench bring-up |
| `[probe-needed]` | Not yet physically verified — confirm before implementation |

Implementers: see [AGENTS.md](../AGENTS.md).
