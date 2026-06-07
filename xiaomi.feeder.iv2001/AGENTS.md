# Agent guide

## Spec structure — four tiers

The `spec/` tree is organized as a dependency chain:

```
Tier 1 — Hardware Invariants        spec/10-hardware/
   |     (constraints, inputs, outputs — the physical PCB)
   v     "given this hardware..."
Tier 2 — Goals / Stories            spec/20-stories/
   |     (what the user wants the device to do)
   v     "to achieve that, the firmware must..."
Tier 3 — Process Descriptions       spec/30-processes/
   |     (detailed mechanism, testable assertions)
   v     "to organize that, the firmware uses..."
Tier 4 — Architecture               spec/40-architecture/
   |     (task model, flash layout, port contracts,
   |      SDK integration, build system)
   v     derived from
Code + Tests                        firmware/
```

There is no separate changelog. Behavioral changes are spec diffs;
commit messages explain "why."

## Evergreen documentation

Spec files are **living reference documents**. They describe the system as
it is today — like a wiki page, not a design diary. Write in present tense;
state facts and behaviors directly.

**Do:**

- "The feeder uses A/B application partitions for OTA."
- "Download writes to the inactive bank."

**Don't:**

- "We previously used streaming OTA; now we use A/B."
- "This was changed from X to Y."
- "Originally…", "used to…", "no longer…"

When a design decision changes, rewrite the affected sections to describe
the current truth in full. Do not annotate superseded approaches inline —
git history is the changelog.

## Reading order

When starting work on a feature or fix:

1. Read `spec/00-overview.md` for project goals and constraints.
2. Read `spec/10-hardware/` — the hardware is your immovable constraint.
3. Read the relevant `spec/20-stories/` file — this is the goal you serve.
4. Read the relevant `spec/30-processes/` files — these are the detailed
   mechanisms to implement. Each process file has a `serves:` field linking
   back to the stories it supports.
5. Read `spec/40-architecture/` — the task model, port contracts, and build
   integration that your code must conform to.

## What belongs at each tier

**Tier 1 (hardware):** Physical facts about the PCB. Pin maps, component
datasheets, power rails, flash chip specs. These change only when the board
revision changes (i.e. never, for this project).

**Tier 2 (stories):** User-facing requirements. Written as "the feeder
should..." or "the user can...". No register addresses, no pin numbers,
no `[tune]` values. Think acceptance criteria.

**Tier 3 (processes):** Engineering-level mechanism descriptions. References
specific pins (P0.0, GPIO17), includes `[tune]` parameters with starting
values, describes sequencing and state transitions. Detailed enough to write
code and test assertions from — but no function signatures, task names, or
module boundaries.

**Tier 4 (architecture):** Software structure decisions. Task model and
priorities, flash partition layout, port interface contracts (function
signatures), SDK integration strategy, and build system. This tier bridges
the gap between "what the firmware does" (Tiers 1–3) and the actual code.
Port contracts in `ports.md` are the primary interface that implementation
code programs against.

## Cross-referencing

Tier 3 process files include a `serves:` field (immediately after the title)
declaring which Tier 2 stories they support. This is many-to-many: a process
like `weighing.md` may serve both `feeding.md` and `monitoring.md`.

## Spec change hygiene

Facts often appear in more than one file. When bench work or a design change
updates a limit, default, or protocol detail, update **every file that
restates that fact** — not just the one you were editing.

### Canonical vs derived

| Kind of fact | Canonical home | Typical derived copies |
|--------------|----------------|------------------------|
| Hardware limits (ranges, command bytes, pin roles) | `10-hardware/` component doc or `pinmap.md` | Tier 3 process that drives the hardware |
| Runtime defaults and `[tune]` starting values | Tier 3 process for that mechanism | `config-store.md` NVDM key table |
| User-visible MQTT fields | `mqtt-protocol.md` | Example JSON payloads, HA discovery table |
| Flash addresses and partition sizes | `40-architecture/partition-layout.md` | `memory_map.h`, linker script, bootloader code |
| Port function signatures | `40-architecture/ports.md` | Port header files in `firmware/ports/` |
| Task priorities and event types | `40-architecture/task-model.md` | `task_def.h`, event enum in source |

Derived copies must match the canonical source. When restating a limit or
default, point to the authority (e.g. "maps to `0x88`–`0x8B`, see
`display-tm1637.md`") so drift is obvious in review.

Do not maintain a separate sync index — inline tags and grep are the
discovery mechanism.

### Before finishing a spec edit

1. **Grep** across `spec/` for the old value, parameter name, NVDM key,
   MQTT field, or component you changed.
2. Update every hit that restates the same fact.
3. If Tier 1 changed, check Tier 3 processes that reference that hardware.
   If a process default changed, check `config-store.md` and `mqtt-protocol.md`.

**Example:** IV2001 brightness has four visible levels (1–4, `0x88`–`0x8B`)
`[probe]`. The limit is canonical in `display-tm1637.md`. These must agree:
`display-presentation.md`, `config-store.md` (`display/brightness`), and
`mqtt-protocol.md` (payload examples and HA discovery range).

## Provenance tags

Every non-trivial hardware or design assertion carries a tag. See the
legend in `spec/README.md`. The most common:

- `[ds:PART §N]` — datasheet reference
- `[probe]` — verified by physical measurement
- `[tune]` — value to be determined on the bench
- `[design]` — our engineering choice

## Source constraints

You implement exclusively from `spec/`. You will not have factory firmware,
decompiler output, or research notes — if a spec gap blocks you, stop and
report it rather than guessing from OEM behavior.

When writing firmware:

- MQTT topics use `petfeeder/<device_id>/` only (see `spec/00-overview.md`).
- No Mi/MIoT/Xiaomi strings in compiled source.
- No OEM flash addresses, ROM symbol names, or RE tooling vocabulary in code.
- Use datasheet-cited constants as written; use `[tune]` values from Tier 3
  specs and refine on the bench — do not invent undocumented timings or thresholds.

Provenance tags in specs are for traceability; see `spec/README.md`.

## Build and SDK constraints

Read `spec/40-architecture/build-integration.md` and
`spec/40-architecture/sdk-reference.md` before touching `firmware/GCC/` or
`tools/`.

1. **No SDK in git.** The MediaTek LinkIt SDK lives only in gitignored
   `external/linkit-sdk-v4.6.2-houndify/`, fetched by `tools/fetch-sdk.sh`.
   Do not commit SDK sources, prebuilt `.a`/`.bin`, or copy example apps into
   the repo. Unity (test harness) is the only third-party C code committed under
   `firmware/`.
2. **MT7682, no PSRAM.** IV2001 module is MHCW05P-B. Build with
   `IC_CONFIG=mt7682`, `PRODUCT_VERSION=7682`, `MTK_NO_PSRAM_ENABLE=y`.
   Linker uses SYSRAM/TCM — not PSRAM regions.
3. **IV2001 board config is ours.** `BOARD_CONFIG=iv2001`. Pinmux in committed
   `firmware/inc/ept_gpio_drv.h` and `firmware/src/ept_gpio_var.c` per
   `spec/10-hardware/pinmap.md`. UART0 console @ GPIO21/22. Never use EVK
   pinmux on feeder hardware.
4. **Scaffold by path.** Reference SDK example files from the fetched tree at
   compile time (`fota_over_wifi` for `sys_init.c`, etc.). Our tree supplies
   `main.c`, `memory_map.h`, linker script, board overlay, patches.
5. **Single SDK.** LinkIt v4.6.2 houndify tree only.
6. **Toolchain.** `arm-none-eabi-gcc` from distro or standalone install — not
   bundled with LinkIt on Linux.

## Test-driven development (mandatory)

All firmware code follows strict **red/green/refactor** TDD. This is not
optional — it is a hard constraint on every implementation task.

### The rule

1. **Read the spec.** Identify the testable assertions in the relevant
   Tier 3 process and Tier 4 port contract.
2. **Write the test first (red).** The test calls port functions, asserts
   expected behavior, and fails because the code does not exist yet.
3. **Write the minimal code to pass (green).** No more, no less.
4. **Refactor.** Clean up duplication, naming, structure — tests must
   still pass.
5. **Repeat** for the next assertion.

Do not write application code without a failing test that demands it.
Do not write a test after the fact to cover code that already exists.

### What to test where

| Test target | Runs on | Asserts | Framework |
|-------------|---------|---------|-----------|
| Host unit tests (`make test-host`) | x86 (gcc/clang) | Tier 3 process logic via fake ports | Unity |
| On-target integration tests | Board (ARM, FreeRTOS) | Tier 2 story outcomes via real HAL | Unity (cross-compiled) |

Host tests are the primary development loop. They run in seconds, need no
hardware, and cover all business logic through port fakes. On-target tests
validate that adapters correctly bind ports to real SDK APIs.

### Traceability

Every test file should name the spec it derives from in a comment at the
top (e.g. `/* Tests: spec/30-processes/ota-flow.md, Verification */`).
This makes it possible to grep from a spec change to the tests that must
be updated.

### Test infrastructure

See `spec/40-architecture/build-integration.md` for the test harness setup,
directory layout, and build commands.

## Deriving code from specs

Implementation in `firmware/` is written exclusively from these specs.
Unit tests assert Tier 3 process behaviors through Tier 4 port contracts.
Integration tests assert Tier 2 story outcomes. When a process changes,
update the spec first, then the tests, then the code — the git diff on
the spec file is the changelog entry. When the architecture changes (new
port, new task, new event), update the Tier 4 spec first.
