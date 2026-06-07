# Updates

## Overview

The feeder supports over-the-air firmware updates. Updates are
user-initiated — there is no auto-update and no cloud dependency.

Flash uses an **A/B dual-bank layout**: two application partitions and a
custom bootloader that boots one bank at a time. The currently running
firmware stays intact until a new image is fully downloaded, verified, and
applied to the inactive bank.

## Update flow

1. The user sends an MQTT command with a firmware download URL.
2. The feeder downloads the image into the inactive application bank and
   reports progress (percentage).
3. After download the feeder verifies image integrity (checksum).
4. If valid, the feeder marks the new bank as active and reboots into it.
5. On first boot after an update, the feeder confirms the new firmware is
   healthy (connectivity check). If that fails within a timeout, the feeder
   automatically rolls back to the previous bank and reboots.

## Safety rules

- The feeder should never be left unbootable by a failed update.
- If download fails or verification fails the update is aborted, the
  inactive bank is discarded, and an error is reported — the running
  firmware is not touched.
- An update is rejected if a dispense is currently in progress.

## Progress reporting

The feeder publishes update status via MQTT:

| State | Meaning |
|-------|---------|
| `idle` | No update in progress. |
| `downloading` | Image download in progress (percentage reported). |
| `applying` | Activating the verified image in the inactive bank and rebooting. |
| `error` | Something went wrong (error detail included). |

## UART recovery

If all else fails (e.g. both firmware slots corrupted), the device can
be recovered via a wired UART connection. This is a last-resort path
and does not require disassembly beyond accessing the debug header.
