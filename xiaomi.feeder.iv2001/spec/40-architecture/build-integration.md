# Build integration and test infrastructure

## Repository rules

1. **SDK stays out of git.** `external/` is gitignored. The Airoha IoT SDK is
   cloned locally by `tools/fetch-sdk.sh`. Never commit SDK files, prebuilt
   libraries, or vendored example app sources. Patches and oddware firmware
   only.
2. **IV2001 board config is committed.** `BOARD_CONFIG=iv2001` with pinmux in
   `firmware/inc/ept_gpio_drv.h` and `firmware/src/ept_gpio_var.c`. Do not
   rely on `mt7682_hdk` EVK pinmux — it conflicts with feeder peripherals.
3. **MT7682, no PSRAM.** `IC_CONFIG=mt7682`, `MTK_NO_PSRAM_ENABLE=y`.
   PSRAM-enabled builds are incompatible with the MHCW05P-B module.
4. **Scaffold by path.** SDK example files (`sys_init.c`, `startup_mt7682.s`,
   etc.) are referenced from the fetched tree at build time, not copied into
   `firmware/`.

## SDK layout

LinkIt SDK `build.sh` expects:

```
project/mt7682_hdk/apps/<name>/GCC/Makefile
```

Fetched to: `external/linkit-sdk-v4.7.1/` (gitignored).

Oddware firmware lives at `xiaomi.feeder.iv2001/firmware/`. `build-env.sh`
creates a real directory (not a symlink — `build.sh` does not follow symlinked
apps):

```
external/.../project/mt7682_hdk/apps/petfeeder/
  src/   inc/   flash/   GCC/   → symlinks into <repo>/firmware/
```

## IV2001 board overlay

| Component | Committed | SDK bridge (gitignored) |
|-----------|-----------|-------------------------|
| `board.mk` | `firmware/board/iv2001/` | `config/board/iv2001/` symlink |
| Pinmux | `firmware/inc/ept_gpio_drv.h`, `src/ept_gpio_var.c` | — |
| Console BSP | — | `driver/board/iv2001/util` → `mt7682_hdk/util` |
| EPT applier | — | `driver/board/iv2001/ept` → `mt7686_hdk/ept` |
| Chip HAL | — | `driver/chip/mt7686/module.mk` (always) |

`build-env.sh` creates board symlinks and applies `firmware/patches/`.

## Onboarding

From `xiaomi.feeder.iv2001/`:

```
./tools/bootstrap.sh           # host tests + SDK fetch + build-env
./tools/bootstrap.sh --host-only   # host tests only
```

Prerequisites: `git`, `make`, `gcc`, `patch` (for SDK patches),
`arm-none-eabi-gcc` (for target builds).

### Build

```
source tools/build-env.sh
cd external/linkit-sdk-v4.7.1
./build.sh mt7682_hdk petfeeder bl
```

Or: `./tools/build-firmware.sh`

Build artifacts: `external/LinkitSDK_OUT/mt7682_hdk/petfeeder/`
(copied to `firmware/flash/` by `build-firmware.sh`)

`firmware/flash/flash_download.cfg` supplies IV2001 2 MB addresses for the
MediaTek Flash Tool (overrides the SDK default 1 MB layout).

## Firmware directory structure

```
firmware/
  board/iv2001/           # board.mk
  GCC/                    # Makefile, mt7682_flash.ld, feature.mk
  inc/                    # memory_map.h, ept_gpio_drv.h
  src/                    # main.c, ept_gpio_var.c, app modules
  flash/                  # flash_download.cfg (IV2001 2 MB)
  patches/                # SDK patches (W25Q16DW combo)
  ports/                  # Port headers
  adapters/               # SDK adapters
  test/                   # Host tests + vendored Unity
```

## SDK patches

Patches live in `firmware/patches/*.patch` and modify the **gitignored**
LinkIt SDK checkout under `external/`. That tree is a **separate git
repository** — `git stash`, `git checkout`, and branch switches in oddware
do **not** revert SDK edits on their own.

`tools/sync-sdk-patches.sh` keeps the SDK deterministic:

1. `git reset --hard HEAD` inside the SDK tree (drops orphaned edits).
2. Apply patches listed in `firmware/patches/series` (one basename per line,
   in dependency order).

A local stamp (`.sdk-patches-stamp`, gitignored) lets `build-env.sh` skip
redundant work when the patch set and SDK HEAD are unchanged and the tree is
clean. `./tools/build-firmware.sh` always re-syncs before building and drops
stale `LinkitSDK_OUT/.../obj` when the SDK was reset.

After `git checkout` or `git pull`, run `source tools/build-env.sh` or install
hooks once: `./tools/install-git-hooks.sh` (post-checkout / post-merge).

**Never** edit tracked SDK files by hand or apply patches manually; add or
remove files under `firmware/patches/` (and `series`) and re-sync.

### Flash combo (W25Q16DW)

JEDEC `0xEF, 0x60, 0x15` is missing from the default `driver/chip/mt7686/`
combo headers. Without it, `hal_flash_init()` fails on IV2001 hardware.

Maintained as `firmware/patches/flash_combo_w25q16dw.patch`.

### Wi-Fi NVDM namespaces

The SDK `wifi_nvdm_config` module seeds its own NVDM groups (`STA`, `AP`,
`common`, …) for radio defaults. Application Wi-Fi credentials use a
separate `wifi` group (`ssid`, `pass`) per
[config-store.md](../30-processes/config-store.md). `sta_auto_connect` is
disabled in `wifi_adapter_stack_init()` so the SDK profile is not used for
association; the connect task reads only the `wifi` group.

`wifi_adapter_wipe_sta_caches()` runs before `wifi_init()` on every boot to
blank the STA profile (`StaFastLink`, `PMK_INFO`, `STA/Ssid`, `STA/WpaPsk`).
This prevents the N9 from loading stale PMKSA / credentials that cause MIC
failures after a warm reboot. Credentials are re-set in `wifi_port_connect()`
via `set_ssid` / `set_psk` / `set_radio(1)` / `reload_setting()`.

### N9 force reset at boot (`connsys_force_n9_reset.patch`)

WDT reboot (triggered by `hal_sys_reboot`) resets only the CM4 core. The N9
coprocessor survives with stale supplicant state (PMKSA, association
context). `connsys_force_n9_reset.patch` asserts `CONNSYS_SW_RST = 0x00`
(held in reset) for 5 ms in `_connsys_init_activate_mcu()` before the
existing release (`= 0x18`). This forces a full N9 RAM clear and ROM
re-init on every boot. `[design]`

### MQTT read short-poll (`mqtt_read_peek_before_select.patch`)

The Paho MQTTClient-C `mqtt_read()` implementation uses a single blocking
`select()` call. On MT7682 with LwIP, a race exists between `rcvevent`
check and semaphore wait: data arriving in the gap is missed until select
timeout. The patch replaces the single select with a short-poll loop
(200 ms intervals until deadline). This eliminates subscribe timeouts and
ensures SUBACK is processed within 200 ms of arrival. `[design]`

### Display boot before Wi-Fi SPI (`mqtt_sys_init_display_boot.patch`)

On MT7682, WFCI (`wfcm_spi.c`) uses GPIO12–16 for SPI to the Wi-Fi N9.
AW9523B shares GPIO14 (reset), GPIO15 (I2C SCL), and GPIO16 (I2C SDA).
`connsys_init()` reclaims those pins; AW9523B I2C NACKs if touched afterward.

Patch `firmware/patches/mqtt_sys_init_display_boot.patch` calls
`display_boot_run()` in SDK `system_init()` after `move_iot_rom_data_to_ram()`
and **before** `log_init()` / `connsys_init()`. Boot self-test blanks the panel
and drops the display rail before WFCI SPI claims GPIO12–16. `main.c` does not
call `display_boot_run()` — the hook lives only in the patched SDK copy.

GPIO13–16 must stay `MODE_NC` / input in `ept_gpio_drv.h`; muxing them at EPT
apply breaks AW9523B I2C (`[probe]` 2026-06-10).

Verify after `source tools/build-env.sh`:

```
grep display_boot_run external/linkit-sdk-v4.7.1/project/mt7682_hdk/apps/mqtt_client/src/sys_init.c
```

Runtime GPIO expander, display, ADC, and UART2 access after Wi-Fi start use
`wfci_bus_port` — see [wfci-bus-arbitration.md](../30-processes/wfci-bus-arbitration.md)
and `ports.md` § `wfci_bus_port`. `wfcm_bus_loan.c` implements
`wfcm_bus_loan_begin()` / `wfcm_bus_loan_end()` (SPI deinit/reinit on
GPIO14–17). `wfci_bus_wifi_spi_active(true)` runs after `connsys_init()` in
`sys_init_wfci_bus.patch`.

GPIO17 is WFCI SPI CS and AUXADC input — jam detection and battery reads use
short `ADC` profile loans, not continuous GPIO17 hold.

### Application logging (`app_log`)

| Setting | Location | Default |
|---------|----------|---------|
| `APP_LOG_LEVEL` | `firmware/GCC/feature.mk` | `debug` |
| `MTK_DEBUG_LEVEL` | `firmware/GCC/feature.mk` | `info` |
| `MTK_MQTT_DEBUG_ENABLE` | `firmware/GCC/feature.mk` | `n` |

Module: `firmware/inc/app_log.h`, `firmware/src/app_log.c`. Linked in
`firmware/GCC/Makefile` and host `firmware/test/Makefile`.

SDK syslog stays at `info`; curated application lines use `app_log` via the same
`log_write()` console path (`io_def.c`). Console hardware is initialized by
`bsp_io_def_uart_init()` in `prvSetupHardware()` before `app_log_init()` —
`app_log` must not deinit/reinit UART0. `app_log_uart_boot.patch` calls
`app_log_init()` after display boot to reset mirror-sink state only.

Host tests compile `app_log.c` with `HOST_TEST` — default sink is `stdout`.
See [app-logging.md](../30-processes/app-logging.md).

`make test-host` runs `check-no-printf` first: bare `printf()` in
`firmware/src/*.c` fails the build (`app_log.c` is exempt). Use `snprintf` for
string assembly; route UART diagnostics through `app_log_*`.

### Remote telnet CLI (`REMOTE_CLI_ENABLE`)

| Setting | Location | Default |
|---------|----------|---------|
| `REMOTE_CLI_ENABLE` | `firmware/GCC/feature.mk` | `y` (current bring-up default); must be `n` before production release |

When `y`, `remote_cli.c` and `console_uart.c` link into the ARM image;
`console_mux.c` is always linked (UART override mux when remote CLI is enabled).
`remote_cli` starts after `EVT_WIFI_STA_READY`. Host tests compile the session
mux and log-mirror logic with `HOST_TEST` (no lwIP).

See [uart-console.md](../30-processes/uart-console.md) § Remote telnet console.

## `tools/build-env.sh`

1. Symlink `petfeeder` app bridge under `project/mt7682_hdk/apps/`.
2. Symlink IV2001 board config into SDK `config/` and `driver/board/`.
3. Sync patches via `tools/sync-sdk-patches.sh` (reset SDK + apply `series`).
4. Wire `arm-none-eabi-gcc` into PATH and SDK-expected symlink paths.

## Test-driven development

Strict red/green/refactor. `[design]`

### Host tests

```
make test-host
```

Unity (vendored in `firmware/test/unity/`). Fakes in `firmware/test/fakes/`.
No FreeRTOS dependency.

### On-target tests

Cross-compiled; triggered via UART CLI (see
[uart-console.md](../30-processes/uart-console.md)) or MQTT. Validate
adapters against real HAL.
