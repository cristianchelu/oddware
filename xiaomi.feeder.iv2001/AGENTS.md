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

The same rule applies to committed `firmware/README.md`, `tools/`, and other
repo docs: **no step checkpoints, bring-up diaries, or plan progress notes.**
Those belong in gitignored agent plans (`.cursor/plans/`) or, only when
there is a durable lesson worth keeping, in gitignored `summaries/` — not in
the tree contributors are expected to read.

**Do not commit:**

- "Step N checkpoint" walkthroughs with expected UART transcripts
- "After this change, run X then look for Y" iteration notes from a session
- Duplicates of plan todos or acceptance criteria already in a plan file

**Do commit:**

- Timeless setup and build commands (what exists, how to invoke it)
- Facts that belong in `spec/` — put them there, not in README padding

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

If step 4 does not yet describe the behavior you are about to implement,
**stop here and write the Tier 3 spec before anything else** — not after
code, not “before commit,” not as a summary of what you built.

## What belongs at each tier

**Tier 1 (hardware):** Physical facts about the PCB. Pin maps, component
datasheets, power rails, flash chip specs. These change only when the board
revision changes (i.e. never, for this project).

**Tier 2 (stories):** User-facing requirements. Written as "the feeder
should..." or "the user can...". No register addresses, no pin numbers,
no `[tune]` values. Think acceptance criteria.

**Tier 3 (processes):** Engineering-level mechanism descriptions. References
specific pins (P0.0, GPIO17), includes `[tune]` parameters with starting
values, describes sequencing and state transitions. Includes every
user-visible or testable interface detail: UART CLI commands and response
strings, MQTT topics and payload fields, captive-portal form fields,
validation rules, timeouts, and state transitions. Detailed enough to write
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
| UART CLI commands and responses | Tier 3 `uart-console.md` | UART CLI handler source |
| Wi-Fi credential validation limits | Tier 3 `uart-console.md` | `wifi_cred.c`, captive portal validation |
| Optional NVDM keys and display states | Tier 3 process for that mechanism (e.g. `uart-console.md` for `pass: (open)`) | `wifi_cred.c`, CLI handlers, provisioning forms |
| SDK vs application NVDM namespaces | `40-architecture/build-integration.md` | `feature.mk`, adapters (do not reuse SDK `STA`/`AP` groups for app config) |
| I2C1 pin roles (GPIO15 SCL, GPIO16 SDA) | `10-hardware/pinmap.md` | `board_gpio_iv2001.h`, `i2c_bus_adapter.c`, `ept_gpio_drv.h` |
| Display boot before `connsys_init()` | `40-architecture/build-integration.md` § Display boot | `mqtt_sys_init_display_boot.patch`, `display_boot.c` |
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
   **Never edit files under `external/` directly.** SDK changes are **patches
   only**: add or update `firmware/patches/<name>.patch`, list it in
   `firmware/patches/series`, run `source tools/build-env.sh` or
   `./tools/sync-sdk-patches.sh`. `./tools/build-firmware.sh` resets and
   re-applies patches automatically before each build.
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
5. **Display boot before Wi-Fi SPI.** AW9523B I2C1 (GPIO15/16) and reset
   (GPIO14) conflict with WFCI SPI on the same pins. `display_boot_run()` runs
   from patched `system_init()` before `connsys_init()`, not from `main.c`.
   Do not move display boot after Wi-Fi init without a pin-arbitration design.
   Hook patch: `firmware/patches/mqtt_sys_init_display_boot.patch`.
6. **SDK patches are reset-synced, not sticky.** The LinkIt tree under
   `external/` is a **separate git repo**. `git stash` / branch checkout in
   oddware does not revert SDK edits. Never apply patches by hand or edit
   tracked SDK files — only add/remove `firmware/patches/*.patch` (and
   `firmware/patches/series`) and sync. Install `./tools/install-git-hooks.sh`
   once so checkout/merge re-syncs when the tree drifts. See
   `spec/40-architecture/build-integration.md` § SDK patches.
7. **Single SDK.** LinkIt v4.6.2 houndify tree only.
8. **Toolchain.** `arm-none-eabi-gcc` from distro or standalone install — not
   bundled with LinkIt on Linux.

## Bench flashing

Linux UART tooling (`tools/iot-flash.sh`, `tools/uart-console.sh`) requires
the user's USB-serial adapter, bench wiring, and manual reset during download.
Agents cannot access that hardware directly.

When a task needs a firmware image on the board — smoke test after a build,
BROM probe, boot-log capture, or reflash — **ask the user** to run the
commands. Reference `README.md` (Flashing) and `spec/10-hardware/flash.md`.
Download starts CODA first; the user resets the feeder (power-cycle or TP15)
within the timed prompt.

## OTA bench loop (agent-run)

When the device already runs OTA-capable A/B firmware and is online on the
same LAN as the broker, agents can **build, deploy, and validate on hardware
themselves** — no manual CODA reset. UART is optional but recommended on the
bench: OTA scripts auto-capture boot logs when the serial port is present.

Full tool reference: `tools/ota/README.md`. Device behaviour:
`spec/30-processes/ota-flow.md`.

### OTA vs UART flash

| Situation | Who runs it |
|-----------|-------------|
| Device online on MQTT, A/B firmware in flash | Agent — `mqtt-ota.sh` (below) |
| First image, bricked bank, bootloader recovery | User — UART flash (above) |

### One-time setup

```bash
cp tools/ota/.env.example tools/ota/.env   # broker host, credentials, UART_DEV
```

Broker must be reachable from both the dev machine and the feeder.
`mosquitto_pub`, `mosquitto_sub`, `python3`, `curl`, and `fuser` are required.

### Iteration cheatsheet

From `xiaomi.feeder.iv2001/`:

```bash
make test-host
source tools/build-env.sh && ./tools/build-firmware.sh
./tools/ota/mqtt-ota.sh --device-id <ID>          # serves inactive-bank .bin, waits for swap
./tools/ota/mqtt-ota.sh --device-id <ID> --skip-build   # when images already built
```

`mqtt-ota.sh` reads the active bank from `petfeeder/<ID>/state`, serves
`petfeeder_a.bin` or `petfeeder_b.bin` from `firmware/flash/` over a local
Range HTTP server, publishes `petfeeder/<ID>/cmd/ota`, and waits for the bank
field to flip. Logs land in `tools/ota/logs/<run-id>/` (HTTP + UART).

### Success criteria

- Script exits 0; hop meta shows `result=OK`.
- `petfeeder/<ID>/state` reports the opposite bank (`A` ↔ `B`).
- On failure: inspect `tools/ota/logs/<run-id>/hop-*-uart.log` for `[ota]` /
  `[mqtt]` lines and `http.log` for stalled Range GETs.

Subscribe to `petfeeder/<ID>/ota/status` for download progress when debugging
interactively.

### Device ID

MQTT topic prefix is `petfeeder/<device_id>/`. The ID is the NVDM
`mqtt/device_id` value when set; otherwise the last six hex digits of the STA
MAC (`spec/30-processes/config-store.md`). Ask the user or read it from an
existing `state` message if unknown — do not assume the example `768722`.

### UART on the bench

Default port: `UART_DEV` in `tools/ota/.env` (typically `/dev/ttyUSB0`).
OTA scripts flock the port during a hop; use `./tools/uart-console.sh` between
runs for interactive CLI (`spec/30-processes/uart-console.md`).

## Ports and adapters (layering)

Application and orchestration code (`firmware/src/`, CLI handlers, credential
logic) depends on **ports** and **port fakes** only — never on LinkIt SDK
headers (`wifi_api.h`, `nvdm.h`, etc.).

| Layer | Location | May include SDK headers? |
|-------|----------|------------------------|
| Port contract | `firmware/ports/` | No |
| Port fake | `firmware/test/fakes/` | No |
| Application logic | `firmware/src/` (e.g. `wifi_cred.c`, `app_wifi_cli.c`) | No |
| Adapter | `firmware/adapters/` | Yes — sole SDK binding site |
| Scaffold reference | SDK paths in `GCC/Makefile` (`sys_init.c`, helpers) | Compiled in; not edited in-tree |

Adapters implement `ports.md` and own stack bring-up for their domain (e.g.
`wifi_adapter_stack_init()`). `main.c` wires modules together; it does not
call SDK Wi-Fi/NVDM APIs directly.

When the SDK ships its own NVDM groups (e.g. `STA` radio profile from
`wifi_nvdm_config`), treat them as HAL defaults. Application config uses
the namespaces in `config-store.md` (`wifi`, `mqtt`, …). See
`build-integration.md` — Wi-Fi NVDM namespaces.

### Display and GPIO expander layering

`[design]` Three concerns, three homes:

| Concern | Modules | Spec |
|---------|---------|------|
| **Infrastructure driver** | `aw9523b.c`, `gpio_expander_port`, `i2c_bus_port` | `gpio-expander-aw9523b.md`, `pinmap.md` |
| **Display driver** | `display_driver.c`, `display_rail.c`, `tm1637.c`, `display_adapter.c` | `display-driver.md` |
| **Display presentation** | `display_boot.c`, future `display_presentation.c` | `display-presentation.md` |

**Dependency rule:** presentation → `display_port` only. Display driver →
`gpio_expander_port` + TM1637 GPIO ops. Presentation **never** includes
`gpio_expander_port`, `tm1637.h`, `aw9523b.h`, or SDK I2C/GPIO headers.

**Anti-patterns:** business logic in `display_driver.c`; TM1637 code touching
I2C/AW9523B; hard-coding boot light test in the driver stack; presentation
calling `gpio_expander_port` or `i2c_bus_port` directly.

## Test-driven development (mandatory)

All firmware code follows strict **spec → red → green → refactor**. This
applies to features, bugfixes, bench discoveries, and plan-driven tasks.
There is no alternate path.

**Debug does not skip the conga.** UART spam, captive-portal misbehaviour,
OTA stalls, and “I’ll add tests after we confirm on hardware” are still
spec → red → green → refactor. Observation on the bench is step 0 (repro
evidence), not permission to patch `firmware/` and backfill tests later.
If you already patched to learn something, **revert or redo**: spec the
correct behaviour, write the failing host test, then re-apply a minimal fix.

### The conga (only valid order)

For **each** testable behavior (one assertion or tightly related group):

1. **Spec** — Tier 3 (and Tier 4 if ports/architecture change) states the
   behavior: CLI strings, MQTT topics, state transitions, error paths,
   logging expectations, NVDM semantics. Commit or stage the spec delta
   **before** the test and code that implement it.
2. **Red** — host test fails against the spec.
3. **Green** — minimal `firmware/` change to pass.
4. **Refactor** — cleanup; tests still pass.

Then the next behavior. Do not batch “implement the whole plan” in code and
spec/tests later.

### Hard stops (no escape hatches)

| Forbidden | Do instead |
|-----------|------------|
| Implement from `.cursor/plans/` or chat acceptance criteria alone | Translate the plan step into Tier 3 spec text first |
| “Reverse-document” / backfill spec from code before commit | Stop; write spec first, then test, then code (revert or redo if already coded) |
| Write tests after code to “cover” what was built | Delete the backward test or treat it as step 2 of the conga on a **new** behavior only after spec exists |
| Bench-debug → patch → flash → spec at end | Observe on bench → **spec delta** → failing test → fix → flash |
| “Debug first, TDD when it works” (any urgency: UART, portal, OTA, crash) | Same conga — urgency changes **deploy** order (flash to verify), not **test** order |
| Spike patch kept because bench looked good | Revert spike or extract testable logic; spec → red → green → re-apply minimal diff |
| Tests added after a debug fix to “lock it in” | Invalid backfill — delete or rewrite as step 2 on a **new** spec’d behaviour |
| Partial spec (“CLI is documented, lifecycle isn’t”) as permission to code | List missing assertions; write them in the canonical Tier 3 file, then conga |
| `#ifdef UNIT_TEST` or duplicate logic to make tests pass | Port fakes and shims (see below) |

If a spec gap blocks you, **stop and report it** — or write the spec
yourself in the same session **before** any application code. Guessing from
OEM behavior, plan wording, or “what we did last time on the bench” is not
allowed.

### Agent plans

`.cursor/plans/` are session checklists — **not** Tier 3. A plan step like
“MQTT + LWT” or “Step 4” is a reminder to edit `mqtt-protocol.md`,
`uart-console.md`, `config-store.md`, etc. **first**.

**Plan-driven workflow:**

1. Read the plan step and map it to Tier 3 files (use the table in
   [No behavior without a process spec](#no-behavior-without-a-process-spec)).
2. Draft or extend those process specs until a reviewer could write tests
   from them alone.
3. Run the conga per behavior group.
4. Use the plan only to tick progress — never as the behavior definition.

### Bench and flash loops

When the user is at the hardware (UART spam, flash failure, reconnect bug):

1. **Diagnose** — capture evidence (log snippet, repro steps). **Stop here**
   before editing `firmware/` unless the Tier 3 spec already states the fix.
2. **Spec** — if the correct behavior is not already in Tier 3, add it now
   (e.g. “idle yield does not drop session”, “countdown ends on `Done.` only”).
3. **Red** — host test that fails with the bug (or documents the invariant).
4. **Green** — fix in `firmware/` or `tools/`.
5. **Deploy** — OTA via `mqtt-ota.sh` when the device is MQTT-online; otherwise
   ask the user to UART-flash.

Flash/OTA is **verification after green**, not a substitute for red. Never ship
a bench-only patch with “tests coming in the next commit”.

Skipping step 2 because “we’ll fix spec before commit” is the same violation
as backfill. The commit must not be the first time Tier 3 mentions the
behavior.

### Corner cases and optional config

Validation-only tests are not enough when storage semantics matter. For each
Tier 3 rule, ask:

1. **Missing key** — NVDM key absent (not just empty string).
2. **Partial write** — user set one field but not another (CLI sets SSID
   before pass).
3. **Display vs connect** — `show` / `is_stored` / `load` may disagree;
   spec must define each (example: open Wi-Fi — SSID alone is enough to
   connect; `pass: (open)` vs `pass: (unset)`).
4. **Shared validation** — UART CLI, captive portal, and MQTT-facing paths
   must use the same limits; canonical rules live in one Tier 3 file, others
   link to it.

Write host tests that exercise the **same API the firmware uses** (e.g.
`wifi_cred_load` through `fake_config_port`), not only validators.

### Host tests: same code, faked boundaries

Host tests must compile and run the **same** `firmware/src/*.c` logic the
device ships. Swap **adapters** for **fakes** at the link boundary — not
`#ifdef UNIT_TEST`, `TEST`, or feature flags that change behavior inside
the module under test.

| Do | Don't |
|----|-------|
| `fakes/port_providers_host.c` implements `config_port_get`, `mqtt_port_get`, `wifi_port_get` | `#ifdef UNIT_TEST` branches in `mqtt_client.c` (or any app module) |
| FreeRTOS/syslog shims in `firmware/test/fakes/shim/` | Copy-paste “test versions” of business logic |
| Extract one iteration (`mqtt_client_step`) and call it from the real task **and** tests | Stub out `mqtt_client_do_connect` only on host |
| Test-only helpers (`mqtt_client_test_reset`) that are never called from firmware | Test-only helpers guarded by macros that also wrap production paths |

Test setup helpers may live in `firmware/src/` if they only reset state or
register callbacks; they must not alter connect/arm/backoff logic. Prefer
driving state through the public API (`mqtt_client_request_connect`,
`mqtt_client_stop`, fake port state) before adding new helpers.

If you think you need `#ifdef` for host tests, you almost certainly need a
port fake, a shim header under `test/fakes/shim/`, or a smaller extracted
function — not a second build of the module.

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

Implementation in `firmware/` is written exclusively from Tier 3 process
specs and Tier 4 port contracts. The git diff on the **spec file must
precede** (or land in the same commit before) the test and code diffs that
implement it — spec is the changelog entry, not an afterword.

When architecture changes (new port, task, event), update Tier 4 first, then
Tier 3 behavior that uses it, then conga.

### No behavior without a process spec

Every testable behavior belongs in a Tier 3 file **before** tests and code.
This includes UART CLI commands, console response strings, validation
limits, MQTT payload fields, session lifecycle, logging policy, and
provisioning form rules — not only “business logic” modules.

| If you are adding… | Spec home (Tier 3) |
|--------------------|--------------------|
| UART CLI command | `uart-console.md` |
| MQTT topic, session rule, or field | `mqtt-protocol.md` |
| Captive-portal form field | `provisioning-flow.md` |
| NVDM key or default | `config-store.md` |
| OTA / bank / flash step | `ota-flow.md`, `partition-layout.md` (Tier 4 for addresses) |
| Flash/download tooling behavior | `10-hardware/flash.md` |

There is **no** “spec debt” or “reverse-document later” path. If code exists
without Tier 3 text, the recovery is: write the spec to match the **intended**
behavior (not a paste of the code), add failing tests, then change code until
tests and spec align — not “document what shipped and call it done.”

## Before finishing a firmware change

Verify the conga actually happened — do not use this list to backfill:

1. **Spec first** — Tier 3 already describes every new/changed behavior;
   grep `spec/` for stale copies; **no** `Step N` or plan-only wording in
   committed `spec/` or `firmware/`.
2. **Tests** — `make test-host` green; new tests existed **before** or
   alongside the code they assert (not a post-hoc coverage pass).
3. **Layering** — no new `#include` of SDK headers under `firmware/src/`.
   Grep `firmware/src/` for `tm1637`, `aw9523b`, or `gpio_expander` includes
   outside the display driver stack; `display_boot.c` must not appear in hits.
4. **Build** — `source tools/build-env.sh` then `./tools/build-firmware.sh`
   when touching adapters, `GCC/`, or patches.
5. **Bench** — when hardware is available and the change is not host-test-only,
   run `mqtt-ota.sh`. UART flash only when OTA is not an option.

If spec and code diverged during the session, **fix order in the branch**:
spec commit (or hunk) before test/code hunks, or split into spec-first PR.
