#!/usr/bin/env node
/* Local preview: bundled UI + mock API — tools/web/README.md */

import fs from 'node:fs';
import http from 'node:http';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

import { createMockApiHandler, createMockApiState } from './mock_api.mjs';

const ROOT = path.dirname(fileURLToPath(import.meta.url));
const PORT = Number(process.env.WEB_UI_PORT || 8765);

let state = createMockApiState();
const getState = () => state;
const setState = (next) => {
    state = next;
};
const handleApi = createMockApiHandler(getState, setState);

function bundleHtml() {
    const logic = fs
        .readFileSync(path.join(ROOT, 'logic.mjs'), 'utf8')
        .replace(/^export /gm, '');
    const html = fs.readFileSync(path.join(ROOT, 'index.html'), 'utf8');
    return html.replace('<!-- INJECT_LOGIC -->', logic);
}

const page = bundleHtml();

const server = http.createServer(async (req, res) => {
    const url = new URL(req.url || '/', `http://127.0.0.1:${PORT}`);

    if (url.pathname.startsWith('/api/')) {
        try {
            await handleApi(req, res, url);
        } catch (err) {
            console.error(err);
            res.writeHead(500);
            res.end('mock error');
        }
        return;
    }

    if (url.pathname === '/' || url.pathname === '/index.html') {
        res.writeHead(200, {
            'Content-Type': 'text/html; charset=utf-8',
            'Cache-Control': 'no-cache',
        });
        res.end(page);
        return;
    }

    res.writeHead(404);
    res.end();
});

server.listen(PORT, '127.0.0.1', () => {
    const gz = Number(process.env.WEB_UI_GZ_BYTES || 0);
    const sizeNote = gz ? ` · ${gz} B gzipped bundle` : '';
    console.log(`Pet Feeder UI preview: http://127.0.0.1:${PORT}/${sizeNote}`);
    console.log('Mock API state resets on restart. Edit index.html/logic.mjs and reload.');
});
