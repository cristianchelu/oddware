# LAN admin web UI

Single-file vanilla HTML/CSS/JS — no bundler. Served gzipped from flash when
`WEB_UI_ENABLE=y` (see `spec/30-processes/web-ui.md`).

| File | Role |
|------|------|
| `index.html` | Markup and DOM wiring |
| `logic.mjs` | Pure helpers (tested, inlined at build) |
| `build.sh` | Inline logic → minify → gzip → `firmware/src/web_ui_gz.c` |
| `dev_server.mjs` | Local preview with mock API |
| `mock_api.mjs` | In-memory `/api/*` for preview |
| `test_logic.mjs` | Host tests (`make test-web`) |

## Build

```bash
./tools/web/build.sh
```

Writes `firmware/src/web_ui_gz.c` (gitignored). Invoked automatically by
`./tools/build-firmware.sh` and `make -C firmware/test test-host`.

Pipeline: inline `logic.mjs` into `index.html`, minify the full page with
`html-minifier-terser` (via `npx`, default version `7.2.0`), then `gzip -9`.
Sources stay readable; only the firmware bundle is minified. The script prints
raw and gzipped byte counts (minified and unminified) after each run.

| Variable | Default | Meaning |
|----------|---------|---------|
| `HTML_MINIFIER_VERSION` | `7.2.0` | Pin for `npx html-minifier-terser@…` |
| `WEB_UI_SKIP_MINIFY` | unset | Set to `1` to gzip the unminified inline bundle |

## Preview (host)

```bash
make -C tools/web preview-web
# or: make preview-web   (from xiaomi.feeder.iv2001/)
```

Opens an **unminified** bundled page at `http://127.0.0.1:8765/` with a mock
feeder API. Edit `index.html` or `logic.mjs`, restart the server, and reload
the browser. Set `WEB_UI_PORT` to change the port. The banner may show gzipped
size from a production `build.sh` run for comparison.

## Tests

```bash
make test-web
```

See `spec/30-processes/web-ui-client.md`.

## Size

Compressed size target: ≤ 12 KB gzipped (`spec/30-processes/web-ui.md` `[tune]`).
Use the minified byte counts from `build.sh` when changing `index.html` or
`logic.mjs`.
