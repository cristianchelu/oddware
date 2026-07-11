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

### CSS: native nesting

`index.html` styles use **native CSS nesting** (`&` for compounds and
pseudos, implicit descendants where the spec allows). This is intentional:
`html-minifier-terser` keeps nested rules in the shipped bundle, so `&` costs
less than repeating selector prefixes — a real gzipped-size win.

- Write **native** nesting only (CSS Nesting Module). No PostCSS, Sass, or
  Less semantics (`&-suffix`, `@nest`, etc.).
- Use `&.modifier` for compound classes (`button.pri`), not a bare `.modifier`
  nested under `button` (that would mean a descendant).
- Tab-radio sibling rules (`#tab-…:checked ~ …`) stay at top level; they do not
  nest cleanly.
- Browsers that load the admin UI must support baseline nesting (2023+). The MCU
  does not parse CSS — only the user's browser does.

Do not flatten nested CSS back to repeated flat selectors without re-running
`build.sh` and comparing gzipped bytes.

Source CSS in `index.html` is formatted for humans (one property per line,
indented nesting). `build.sh` minifies it — do not compress the source to save
flash bytes.

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
