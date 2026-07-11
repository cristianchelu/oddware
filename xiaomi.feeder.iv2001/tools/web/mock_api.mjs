/* In-memory HTTP API for local UI preview — spec/30-processes/web-ui.md */

import { indicesToMask, weekdayFromLocalTime } from './logic.mjs';

const TERMINAL_SLOT_STATES = new Set(['dispensing', 'dispensed', 'failed', 'skipped_full']);

function pad2(n) {
    return String(n).padStart(2, '0');
}

function slotKey(hour, min) {
    return `${hour}:${pad2(min)}`;
}

function slotMinutes(hour, min) {
    return hour * 60 + min;
}

function parseLocalTime(local_time) {
    if (!local_time || local_time.length < 16) {
        return null;
    }
    const wdayMon0 = weekdayFromLocalTime(local_time);
    const [hour, min] = local_time.slice(11, 16).split(':').map(Number);
    if (wdayMon0 === null || Number.isNaN(hour) || Number.isNaN(min)) {
        return null;
    }
    return { wdayMon0, hour, min };
}

function slotAppliesToday(repeat_days, wdayMon0) {
    return Array.isArray(repeat_days) && repeat_days.includes(wdayMon0);
}

/** Mock-only: mirrors firmware schedule state — scheduler-engine.md. Not in logic.mjs / flash bundle. */
export function effectiveSlotState(slot, state) {
    if (slot.terminal && TERMINAL_SLOT_STATES.has(slot.terminal)) {
        return slot.terminal;
    }
    if (!state.time_synced || !state.local_time) {
        return 'pending';
    }
    const now = parseLocalTime(state.local_time);
    if (!now) {
        return 'pending';
    }
    const today = slotAppliesToday(slot.repeat_days, now.wdayMon0);
    const slotMin = slotMinutes(slot.hour, slot.min);
    const nowMin = slotMinutes(now.hour, now.min);
    const futureToday = today && slotMin > nowMin;
    const pastToday = today && slotMin < nowMin;

    if (slot.skip_today) {
        return futureToday ? 'to_be_skipped' : 'skipped';
    }
    if (!state.today_enabled && futureToday && today) {
        return 'skipped';
    }
    if (pastToday && slot.enabled && state.schedule_enabled && !slot.skip_today) {
        return 'skipped';
    }
    return 'pending';
}

function slotToday(slot, state) {
    if (!state.time_synced || !state.local_time) {
        return false;
    }
    const now = parseLocalTime(state.local_time);
    return now ? slotAppliesToday(slot.repeat_days, now.wdayMon0) : false;
}

function defaultSlots() {
    return [
        {
            time: '7:00',
            hour: 7,
            min: 0,
            repeat_days: [0, 1, 2, 3, 4],
            g: 30,
            enabled: true,
            skip_today: false,
            terminal: null,
        },
        {
            time: '18:30',
            hour: 18,
            min: 30,
            repeat_days: [0, 1, 2, 3, 4, 5, 6],
            g: 40,
            enabled: true,
            skip_today: false,
            terminal: null,
        },
    ];
}

export function createMockApiState(overrides = {}) {
    return {
        time_synced: true,
        local_time: '2026-07-09 10:00:00',
        hopper: 'normal',
        dispense_busy: false,
        schedule_enabled: true,
        today_enabled: true,
        bowl_weight: '42.3',
        bowl_error: false,
        battery: '85',
        mains: 'ON',
        feed_mode: 'compensated',
        tz_rule: 'UTC-5',
        slots: defaultSlots(),
        ...overrides,
    };
}

function computeNext(slots, state) {
    if (!state.schedule_enabled) {
        return null;
    }
    const pending = slots
        .filter((s) => s.enabled && effectiveSlotState(s, state) === 'pending')
        .sort((a, b) => a.hour - b.hour || a.min - b.min)[0];
    if (!pending) {
        return null;
    }
    const now = parseLocalTime(state.local_time);
    let in_min = 47;
    if (now) {
        const delta = slotMinutes(pending.hour, pending.min) - slotMinutes(now.hour, now.min);
        in_min = Math.max(0, delta);
    }
    return { hour: pending.hour, min: pending.min, g: pending.g, in_min };
}

function statusPayload(state) {
    const next = computeNext(state.slots, state);
    const body = {
        time_synced: state.time_synced,
        hopper: state.hopper,
        dispense_busy: state.dispense_busy,
        schedule_enabled: state.schedule_enabled,
        today_enabled: state.today_enabled,
    };
    if (state.time_synced) {
        body.local_time = state.local_time;
    }
    if (state.bowl_weight !== undefined) {
        body.bowl_weight = state.bowl_weight;
    }
    if (state.bowl_error !== undefined) {
        body.bowl_error = state.bowl_error;
    }
    if (state.battery !== undefined) {
        body.battery = state.battery;
    }
    if (state.mains !== undefined) {
        body.mains = state.mains;
    }
    if (next) {
        body.next = next;
    }
    return body;
}

function schedulePayload(state) {
    return {
        enabled: state.schedule_enabled,
        today_enabled: state.today_enabled,
        schedule: state.slots.map((s) => ({
            time: s.time,
            repeat_days: s.repeat_days,
            g: s.g,
            enabled: s.enabled,
            today: slotToday(s, state),
            state: effectiveSlotState(s, state),
        })),
    };
}

function json(res, status, obj) {
    const body = JSON.stringify(obj);
    res.writeHead(status, {
        'Content-Type': 'application/json',
        'Content-Length': Buffer.byteLength(body),
    });
    res.end(body);
}

function text(res, status, body) {
    res.writeHead(status, {
        'Content-Type': 'text/plain',
        'Content-Length': Buffer.byteLength(body),
    });
    res.end(body);
}

function readBody(req) {
    return new Promise((resolve, reject) => {
        const chunks = [];
        req.on('data', (c) => chunks.push(c));
        req.on('end', () => resolve(Buffer.concat(chunks).toString('utf8')));
        req.on('error', reject);
    });
}

function findSlot(state, hour, min) {
    return state.slots.find((s) => s.hour === hour && s.min === min);
}

export function createMockApiHandler(getState, setState) {
    return async function handleMockApi(req, res, url) {
        const path = url.pathname;
        const method = req.method || 'GET';

        if (method === 'GET' && path === '/api/status') {
            return json(res, 200, statusPayload(getState()));
        }
        if (method === 'GET' && path === '/api/schedule/state') {
            return json(res, 200, schedulePayload(getState()));
        }
        if (method === 'GET' && path === '/api/feed/mode') {
            return text(res, 200, getState().feed_mode);
        }
        if (method === 'GET' && path === '/api/config') {
            return json(res, 200, { tz_rule: getState().tz_rule });
        }

        if (method !== 'POST') {
            res.writeHead(405);
            return res.end();
        }

        const raw = await readBody(req);
        let body = raw;
        if (raw.startsWith('{')) {
            try {
                body = JSON.parse(raw);
            } catch {
                return json(res, 400, { ok: false, error: 'bad_json' });
            }
        }

        const state = getState();

        if (path === '/api/dispense') {
            if (state.dispense_busy) {
                return json(res, 200, { ok: false, error: 'busy' });
            }
            setState({ ...state, dispense_busy: true });
            setTimeout(() => setState({ ...getState(), dispense_busy: false }), 2500);
            return json(res, 200, { ok: true });
        }

        if (path === '/api/schedule/enable') {
            setState({ ...state, schedule_enabled: !!body.enabled });
            return json(res, 200, { ok: true });
        }

        if (path === '/api/schedule/today') {
            setState({ ...state, today_enabled: !!body.enabled });
            return json(res, 200, { ok: true });
        }

        if (path === '/api/feed/mode') {
            const mode = typeof body === 'string' ? body.trim() : '';
            if (mode !== 'open_loop' && mode !== 'compensated') {
                return json(res, 200, { ok: false, error: 'rejected' });
            }
            setState({ ...state, feed_mode: mode });
            return json(res, 200, { ok: true });
        }

        if (path === '/api/config') {
            if (body && body.tz_rule !== undefined) {
                setState({ ...state, tz_rule: body.tz_rule });
            }
            return json(res, 200, { ok: true });
        }

        if (path === '/api/schedule/set') {
            const hour = Number(body.hour);
            const min = Number(body.min);
            const key = slotKey(hour, min);
            const repeat_days = Array.isArray(body.repeat_days) ? body.repeat_days : [];
            const entry = {
                time: `${hour}:${pad2(min)}`,
                hour,
                min,
                repeat_days,
                g: Number(body.g),
                enabled: !!body.enabled,
                skip_today: false,
                terminal: null,
            };
            const slots = state.slots.filter((s) => !(s.hour === hour && s.min === min));
            slots.push(entry);
            slots.sort((a, b) => a.hour - b.hour || a.min - b.min);
            setState({ ...state, slots });
            return json(res, 200, { ok: true });
        }

        if (path === '/api/schedule/delete') {
            const hour = Number(body.hour);
            const min = Number(body.min);
            setState({
                ...state,
                slots: state.slots.filter((s) => !(s.hour === hour && s.min === min)),
            });
            return json(res, 200, { ok: true });
        }

        if (path === '/api/schedule/toggle') {
            const hour = Number(body.hour);
            const min = Number(body.min);
            const slot = findSlot(state, hour, min);
            if (!slot) {
                return json(res, 200, { ok: false, error: 'not_found' });
            }
            slot.enabled = !slot.enabled;
            setState({ ...state });
            return json(res, 200, { ok: true });
        }

        if (path === '/api/schedule/skip') {
            const hour = Number(body.hour);
            const min = Number(body.min);
            const slot = findSlot(state, hour, min);
            if (slot) {
                slot.skip_today = !!body.skip;
            }
            setState({ ...state });
            return json(res, 200, { ok: true });
        }

        res.writeHead(404);
        res.end();
    };
}

export function maskFromRepeatDays(repeat_days) {
    return indicesToMask(repeat_days || []);
}
