# MQTT bench

CLI for publishing retained MQTT topics and Home Assistant discovery payloads
to a real broker — useful for validating HA entities and JSON shape without
reflashing the device.

Broker credentials come from `tools/ota/.env` (same as OTA scripts).

## Setup

```bash
cp tools/ota/.env.example tools/ota/.env   # edit MQTT_HOST, credentials
export DEVICE_ID=ddeeff                    # bench placeholder or live device id
chmod +x tools/mqtt/mqtt-bench.sh
```

Requires `mosquitto_pub` and `mosquitto_sub`.

## Food bowl error slice

Run from `xiaomi.feeder.iv2001/`:

```bash
./tools/mqtt/mqtt-bench.sh session online
./tools/mqtt/mqtt-bench.sh ota idle A
./tools/mqtt/mqtt-bench.sh ha discovery bowl_error
./tools/mqtt/mqtt-bench.sh state bowl_error on
./tools/mqtt/mqtt-bench.sh state bowl_error off
```

### HA checklist

1. **Bowl error** entity appears; `state bowl_error on/off` toggles problem state.
2. `session offline` → unavailable; `session online` → restores.
3. `ota idle A` / `ota idle B` — `mosquitto_sub` shows `bank` on `ota/status`.
4. Retained `state` is `{"bowl_error":...}` only — no `online` or `bank` fields.
5. When changing payload shape, update matching files in `payloads/` and
   `spec/30-processes/mqtt-protocol.md`.

### Troubleshooting

**No new entity after `ha discovery bowl_error`:**

1. Use the **same `DEVICE_ID`** as your existing Dispense button entity
   (check its MQTT topic or `unique_id` in HA → entity → Settings).
2. Dry-run JSON: `./tools/mqtt/mqtt-bench.sh verify ha bowl_error`
3. Confirm the message landed:
   `mosquitto_sub -h $MQTT_HOST -u $MQTT_USER -P $MQTT_PASS -t 'homeassistant/binary_sensor/petfeeder_+/bowl_error/config' -C 1 -v`
4. Re-publish discovery after fixing payload (bench validates JSON before publish).
5. In HA: Settings → Devices & services → MQTT → Configure → Subscribe to
   `homeassistant/#` briefly and look for errors in the logs.

If the entity appears under **Pet Feeder &lt;other-id&gt;** you used the wrong
`DEVICE_ID` — set `export DEVICE_ID=...` to match the live feeder.

## Battery slice

Validate unavailable vs known SOC (requires `session online` first):

```bash
export DEVICE_ID=768722   # your feeder id
./tools/mqtt/mqtt-bench.sh ha discovery battery
./tools/mqtt/mqtt-bench.sh battery unknown   # HA → unavailable
./tools/mqtt/mqtt-bench.sh battery 75        # HA → 75 %
```

Do **not** use an empty payload for unknown — Mosquitto drops zero-length
retains. Payload files live in `payloads/battery-unknown` and `payloads/ha-battery.json`.

## Entity categories (Diagnostics / Configuration)

Bench `entity_category` before firmware ships. Use the same `DEVICE_ID` as your
live feeder.

```bash
export DEVICE_ID=<your feeder id>
./tools/mqtt/mqtt-bench.sh session online

# Diagnostic section
./tools/mqtt/mqtt-bench.sh ha discovery battery_voltage
./tools/mqtt/mqtt-bench.sh ha discovery device_timezone
./tools/mqtt/mqtt-bench.sh battery_voltage 4200

# Configuration section
./tools/mqtt/mqtt-bench.sh ha discovery weight_compensation
./tools/mqtt/mqtt-bench.sh ha discovery overfill_protection
./tools/mqtt/mqtt-bench.sh ha discovery overfill_threshold_g
./tools/mqtt/mqtt-bench.sh feed_mode compensated
```

### HA checklist

1. Device page shows **Diagnostics** card with battery voltage and device timezone.
2. **Configuration** card shows weight compensation, overfill protection, and overfill threshold.
3. Primary entities (bowl weight, dispense, battery, etc.) stay on the main device view.
4. Dry-run any payload: `./tools/mqtt/mqtt-bench.sh verify ha battery_voltage`
5. Re-publish discovery after editing `payloads/ha-*.json`; reload MQTT integration if HA does not update.

**Updating `entity_category` on entities HA already discovered:** republishing discovery
alone is not enough — Home Assistant keeps the original category in the entity
registry. Clear and republish:

```bash
./tools/mqtt/mqtt-bench.sh ha rediscover weight_compensation
# or all diagnostic + config entities at once:
./tools/mqtt/mqtt-bench.sh ha rediscover-categories
```

Optional wait before republish (default 5 s): `MQTT_BENCH_HA_REDISCOVER_WAIT_S=8`.

Category mapping is canonical in `spec/30-processes/mqtt-protocol.md` § Entity categories.

## Generic commands

```bash
./tools/mqtt/mqtt-bench.sh pub connection online --retain
./tools/mqtt/mqtt-bench.sh pub state -f payloads/state-ok.json --retain
./tools/mqtt/mqtt-bench.sh sub 'petfeeder/+/#'
./tools/mqtt/mqtt-bench.sh clean --slice all
```

## Adding a topic

1. Add a versioned JSON file under `payloads/`.
2. Document the field in `spec/30-processes/mqtt-protocol.md`.
3. Add a named recipe to `mqtt-bench-lib.sh` and wire it in `mqtt-bench.sh`.
