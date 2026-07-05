# LAN admin web UI

Single-file vanilla HTML/CSS/JS — no bundler. Served gzipped from flash when
`WEB_UI_ENABLE=y` (see `spec/30-processes/web-ui.md`).

## Build

```bash
./tools/web/build.sh
```

Writes `firmware/src/web_ui_gz.c` (gitignored). Invoked automatically by
`./tools/build-firmware.sh` and `make -C firmware/test test-host`.

The script prints raw and gzipped byte counts after each run.

## Size

Compressed size target: ≤ 12 KB gzipped (`spec/30-processes/web-ui.md` `[tune]`).
Use the byte counts from `build.sh` when changing `index.html`.
