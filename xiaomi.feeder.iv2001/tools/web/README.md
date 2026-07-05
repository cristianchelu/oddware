# LAN admin web UI

Single-file vanilla HTML/CSS/JS — no bundler. Served gzipped from flash when
`WEB_UI_ENABLE=y` (see `spec/30-processes/web-ui.md`).

| File | Role |
|------|------|
| `index.html` | Markup and DOM wiring |
| `logic.mjs` | Pure helpers (tested, inlined at build) |
| `build.sh` | Inline logic → gzip → `firmware/src/web_ui_gz.c` |
| `test_logic.mjs` | Host tests (`make test-web`) |

## Build

```bash
./tools/web/build.sh
```

Writes `firmware/src/web_ui_gz.c` (gitignored). Invoked automatically by
`./tools/build-firmware.sh` and `make -C firmware/test test-host`.

The script prints raw and gzipped byte counts after each run.

## Tests

```bash
make test-web
```

See `spec/30-processes/web-ui-client.md`.

## Size

Compressed size target: ≤ 12 KB gzipped (`spec/30-processes/web-ui.md` `[tune]`).
Use the byte counts from `build.sh` when changing `index.html` or `logic.mjs`.
