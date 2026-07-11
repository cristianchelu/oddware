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

export function buildScheduleToggleBody(hour, min) {
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

export function buildFeedOverfillBody({ enabled, threshold_g }) {
    const body = {};
    if (enabled !== undefined) {
        body.enabled = Boolean(enabled);
    }
    if (threshold_g !== undefined && threshold_g !== '') {
        body.threshold_g = Number(threshold_g);
    }
    return body;
}

export function formatNextFeed(next) {
    if (!next) {
        return 'No upcoming feed';
    }
    const mm = String(next.min).padStart(2, '0');
    return `${next.hour}:${mm} · ${next.g}g · ${next.in_min}m`;
}

export function formatNextFeedParts(next) {
    if (!next) {
        return { headline: 'Next feed · nothing scheduled', detail: null };
    }
    const hh = String(next.hour).padStart(2, '0');
    const mm = String(next.min).padStart(2, '0');
    const when = next.in_min === 0 ? 'now' : `in ${next.in_min} min`;
    return {
        headline: `Next feed · ${when}`,
        detail: `${hh}:${mm} · ${next.g}g`,
    };
}

export const WEEKDAY_MASK = indicesToMask([0, 1, 2, 3, 4]);
export const ALL_DAYS_MASK = 127;

export function sortSlotsByTime(slots) {
    return [...slots].sort((a, b) => {
        const [ah, am] = a.time.split(':').map(Number);
        const [bh, bm] = b.time.split(':').map(Number);
        return ah !== bh ? ah - bh : am - bm;
    });
}

export function formatEditorTitle(time) {
    return time ? `Edit ${time}` : 'New feeding time';
}

export function formatStatusMessage(st) {
    const time = st.time_synced ? st.local_time : 'time not synced';
    return `${time} | hopper ${st.hopper} | busy ${st.dispense_busy}`;
}

export function formatRefreshTime(date) {
    const d = date instanceof Date ? date : new Date(date);
    const h = d.getHours();
    const m = String(d.getMinutes()).padStart(2, '0');
    const s = String(d.getSeconds()).padStart(2, '0');
    return `${h}:${m}:${s}`;
}

export function formatMetaLine(st, refreshedAt) {
    const hop = formatHopper(st.hopper).toLowerCase();
    const busy = st.dispense_busy ? 'feeding' : 'idle';
    return `${formatRefreshTime(refreshedAt)} · hopper ${hop} · ${busy}`;
}

export function formatStatusAlerts(st) {
    const out = [];
    if (!st.time_synced) {
        out.push({ text: 'Time not synced', tone: 'warn' });
    }
    if (st.bowl_error) {
        out.push({ text: 'Bowl problem', tone: 'bad' });
    }
    return out;
}

export function formatBowlWeightWire(wire) {
    if (wire === undefined) {
        return '—';
    }
    if (wire === '') {
        return 'Unknown';
    }
    return `${wire} g`;
}

export function bowlWeightTone(wire) {
    if (wire === undefined) {
        return 'muted';
    }
    if (wire === '') {
        return 'warn';
    }
    return 'ok';
}

export function formatHopperLevel(hopper) {
    switch (hopper) {
        case 'empty':
            return 'Empty';
        case 'low':
            return 'Low';
        default:
            return 'Normal';
    }
}

export function formatBowlHealth(bowl_error) {
    if (bowl_error === undefined) {
        return '—';
    }
    return bowl_error ? 'Problem' : 'OK';
}

export function bowlHealthTone(bowl_error) {
    if (bowl_error === undefined) {
        return 'muted';
    }
    return bowl_error ? 'bad' : 'ok';
}

export function formatBatteryWire(wire) {
    if (wire === undefined) {
        return '—';
    }
    if (wire === 'unknown') {
        return 'Unknown';
    }
    return `${wire}%`;
}

export function formatMainsWire(wire) {
    if (wire === undefined) {
        return '—';
    }
    return wire === 'ON' ? 'Mains' : 'Battery';
}

export function statusStateRows(st) {
    const rows = [
        {
            label: 'Food in bowl',
            value: formatBowlWeightWire(st.bowl_weight),
            tone: bowlWeightTone(st.bowl_weight),
        },
        {
            label: 'Hopper',
            value: formatHopperLevel(st.hopper),
            tone: hopperTone(st.hopper),
        },
        {
            label: 'Activity',
            value: formatBusy(st.dispense_busy),
            tone: busyTone(st.dispense_busy),
        },
        {
            label: 'Bowl',
            value: formatBowlHealth(st.bowl_error),
            tone: bowlHealthTone(st.bowl_error),
        },
    ];
    if (st.battery !== undefined) {
        rows.push({
            label: 'Battery',
            value: formatBatteryWire(st.battery),
            tone: st.battery === 'unknown' ? 'warn' : 'ok',
        });
    }
    if (st.mains !== undefined) {
        rows.push({
            label: 'Power',
            value: formatMainsWire(st.mains),
            tone: st.mains === 'ON' ? 'ok' : 'muted',
        });
    }
    return rows;
}

export function formatDeviceTime(st) {
    if (!st.time_synced || !st.local_time) {
        return '—';
    }
    return st.local_time;
}

export function formatHopper(hopper) {
    switch (hopper) {
        case 'empty':
            return 'Empty';
        case 'low':
            return 'Low';
        default:
            return 'OK';
    }
}

export function hopperTone(hopper) {
    switch (hopper) {
        case 'empty':
            return 'bad';
        case 'low':
            return 'warn';
        default:
            return 'ok';
    }
}

export function formatBusy(busy) {
    return busy ? 'Feeding' : 'Idle';
}

export function busyTone(busy) {
    return busy ? 'warn' : 'ok';
}

export function formatFeedModeLabel(mode) {
    return mode === 'compensated' ? 'Compensated' : 'Open loop';
}

export function formatSlotDayLetters(repeat_days) {
    return (repeat_days || []).map((d) => DAYS[d][0]).join('');
}

export function formatSlotClock(time) {
    const [h, m] = String(time).split(':');
    return `${String(h).padStart(2, '0')}:${String(m).padStart(2, '0')}`;
}

export function weekdayFromLocalTime(local_time) {
    if (!local_time || local_time.length < 10) {
        return null;
    }
    const parts = local_time.slice(0, 10).split('-').map(Number);
    if (parts.length !== 3 || parts.some((n) => Number.isNaN(n))) {
        return null;
    }
    const [year, month, day] = parts;
    const sun0 = new Date(year, month - 1, day).getDay();
    return sun0 === 0 ? 6 : sun0 - 1;
}

export function slotDayBadges(repeat_days, todayIndex = null) {
    const set = new Set(repeat_days || []);
    return DAYS.map((label, i) => ({
        letter: label[0],
        label,
        on: set.has(i),
        today: todayIndex !== null && i === todayIndex,
    }));
}

export function formatSlotDays(repeat_days) {
    if (!repeat_days || !repeat_days.length) {
        return '—';
    }
    return repeat_days.map((d) => DAYS[d]).join(', ');
}

export function formatSlotState(state) {
    if (!state) {
        return '—';
    }
    return String(state).replace(/_/g, ' ');
}

export function slotStateTone(state) {
    switch (state) {
        case 'dispensed':
            return 'ok';
        case 'pending':
        case 'to_be_skipped':
            return 'warn';
        case 'missed':
            return 'bad';
        case 'skipped':
        case 'skipped_full':
            return 'muted';
        default:
            return 'muted';
    }
}

/** Slot clock `HH:MM` or `H:MM` → minutes from midnight, or null. */
export function slotMinutesFromClock(time) {
    const parts = String(time).split(':');
    if (parts.length !== 2) {
        return null;
    }
    const h = Number(parts[0]);
    const m = Number(parts[1]);
    if (Number.isNaN(h) || Number.isNaN(m)) {
        return null;
    }
    return h * 60 + m;
}

/** True when slot applies today and its time is before feeder `local_time`. */
export function slotIsPastToday(slot, local_time) {
    if (!slot?.today || !local_time) {
        return false;
    }
    const nowMin = slotMinutesFromClock(local_time.slice(11, 16));
    const slotMin = slotMinutesFromClock(slot.time);
    if (nowMin === null || slotMin === null) {
        return false;
    }
    return slotMin < nowMin;
}

const SKIP_HIDDEN_STATES = new Set(['dispensed', 'failed', 'skipped_full', 'dispensing', 'skipped']);

/**
 * Skip-today row action for a slot card, or null when no action applies.
 * Unskip only for future `to_be_skipped`; past rows hide the control.
 */
export function slotSkipControl(slot, local_time) {
    if (!slot?.today || slotIsPastToday(slot, local_time)) {
        return null;
    }
    if (SKIP_HIDDEN_STATES.has(slot.state)) {
        return null;
    }
    if (slot.state === 'to_be_skipped') {
        return { label: 'Unskip', skip: false };
    }
    return { label: 'Skip today', skip: true };
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
