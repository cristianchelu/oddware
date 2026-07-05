/* Pure helpers — spec/30-processes/web-ui-client.md */

export const DAYS = ['Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat', 'Sun'];

export function indicesToMask(indices) {
    let mask = 0;
    for (const i of indices) {
        mask |= 1 << i;
    }
    return mask;
}

export function maskToIndices(mask) {
    const out = [];
    for (let i = 0; i < 7; i++) {
        if (mask & (1 << i)) {
            out.push(i);
        }
    }
    return out;
}

export function readMaskFromCheckedValues(values) {
    let mask = 0;
    for (const v of values) {
        mask |= 1 << Number(v);
    }
    return mask;
}

export function validateScheduleMask(mask) {
    if (!mask) {
        return { ok: false, error: 'Pick at least one day' };
    }
    return { ok: true };
}

export function buildScheduleSetBody({ hour, min, mask, g, enabled }) {
    return {
        hour: Number(hour),
        min: Number(min),
        repeat_days: maskToIndices(mask),
        g: Number(g),
        enabled: Boolean(enabled),
    };
}

export function buildScheduleDeleteBody(hour, min) {
    return { hour: Number(hour), min: Number(min) };
}

export function buildScheduleSkipBody(hour, min, skip = true) {
    return { hour: Number(hour), min: Number(min), skip: Boolean(skip) };
}

export function buildDispenseBody(g) {
    return { g: Number(g) };
}

export function buildScheduleEnableBody(enabled) {
    return { enabled: Boolean(enabled) };
}

export function buildScheduleTodayBody(enabled) {
    return { enabled: Boolean(enabled) };
}

export function buildConfigTzBody(tz_rule) {
    return { tz_rule };
}

export function formatNextFeed(next) {
    if (!next) {
        return 'No upcoming feed';
    }
    const mm = String(next.min).padStart(2, '0');
    return `Next: ${next.hour}:${mm} ${next.g}g in ${next.in_min}m`;
}

export function formatStatusMessage(st) {
    const time = st.time_synced ? st.local_time : 'time not synced';
    return `${time} | hopper ${st.hopper} | busy ${st.dispense_busy}`;
}

export function formatSlotDayLetters(repeat_days) {
    return (repeat_days || []).map((d) => DAYS[d][0]).join('');
}

export function parseApiResponse(text) {
    try {
        return JSON.parse(text);
    } catch {
        return text;
    }
}

export function apiContentType(body) {
    if (body === undefined) {
        return undefined;
    }
    return body.startsWith('{') ? 'application/json' : 'text/plain';
}

export function mutationMessage(r, okText, failPrefix) {
    if (r && r.ok) {
        return { text: okText, ok: true };
    }
    const err = r && r.error ? r.error : 'unknown';
    if (failPrefix) {
        return { text: `${failPrefix}: ${err}`, ok: false };
    }
    return { text: err, ok: false };
}
