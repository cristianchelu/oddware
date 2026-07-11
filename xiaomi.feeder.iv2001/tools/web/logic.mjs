/* Pure helpers — spec/30-processes/web-ui-client.md */

export const DAYS = ['Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat', 'Sun'];
export const ALL_DAYS_MASK = 127;

const HOPPER = { empty: ['Empty', 'bad'], low: ['Low', 'warn'], normal: ['Normal', 'ok'] };
const SLOT_TONE = {
    dispensed: 'ok',
    pending: 'warn',
    to_be_skipped: 'warn',
    missed: 'bad',
    skipped: 'muted',
    skipped_full: 'muted',
};
const SKIP_HIDE = new Set(['dispensed', 'failed', 'skipped_full', 'dispensing', 'skipped']);

export function indicesToMask(indices) {
    let mask = 0;
    for (const i of indices) mask |= 1 << i;
    return mask;
}

export function maskToIndices(mask) {
    const out = [];
    for (let i = 0; i < 7; i++) if (mask & (1 << i)) out.push(i);
    return out;
}

export function readMaskFromCheckedValues(values) {
    let mask = 0;
    for (const v of values) mask |= 1 << Number(v);
    return mask;
}

export function validateScheduleMask(mask) {
    return mask ? { ok: true } : { ok: false, error: 'Pick at least one day' };
}

function schedTime(hour, min) {
    return { hour: Number(hour), min: Number(min) };
}

export function buildScheduleSetBody({ hour, min, mask, g, enabled }) {
    return { ...schedTime(hour, min), repeat_days: maskToIndices(mask), g: Number(g), enabled: Boolean(enabled) };
}

export const buildScheduleDeleteBody = schedTime;
export const buildScheduleToggleBody = schedTime;

export function buildScheduleSkipBody(hour, min, skip = true) {
    return { ...schedTime(hour, min), skip: Boolean(skip) };
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
    if (enabled !== undefined) body.enabled = Boolean(enabled);
    if (threshold_g !== undefined && threshold_g !== '') body.threshold_g = Number(threshold_g);
    return body;
}

export function formatNextFeedParts(next) {
    if (!next) return { headline: 'Next feed · nothing scheduled', detail: null };
    const hh = String(next.hour).padStart(2, '0');
    const mm = String(next.min).padStart(2, '0');
    const when = next.in_min === 0 ? 'now' : `in ${next.in_min} min`;
    return { headline: `Next feed · ${when}`, detail: `${hh}:${mm} · ${next.g}g` };
}

export function sortSlotsByTime(slots) {
    return [...slots].sort((a, b) => {
        const [ah, am] = a.time.split(':').map(Number);
        const [bh, bm] = b.time.split(':').map(Number);
        return ah !== bh ? ah - bh : am - bm;
    });
}

export function formatStatusAlerts(st) {
    const out = [];
    if (!st.time_synced) out.push({ text: 'Time not synced', tone: 'warn' });
    if (st.bowl_error) out.push({ text: 'Bowl problem', tone: 'bad' });
    return out;
}

function wireText(wire, empty, unknown) {
    if (wire === undefined) return '—';
    if (wire === '') return unknown;
    return `${wire} g`;
}

function wireTone(wire, emptyTone = 'muted') {
    if (wire === undefined) return emptyTone;
    if (wire === '') return 'warn';
    return 'ok';
}

export function formatBowlWeightWire(wire) {
    return wireText(wire, '—', 'Unknown');
}

export function bowlWeightTone(wire) {
    return wireTone(wire);
}

export function formatHopperLevel(hopper) {
    return (HOPPER[hopper] || HOPPER.normal)[0];
}

export function hopperTone(hopper) {
    return (HOPPER[hopper] || HOPPER.normal)[1];
}

export function formatBowlHealth(bowl_error) {
    if (bowl_error === undefined) return '—';
    return bowl_error ? 'Problem' : 'OK';
}

export function bowlHealthTone(bowl_error) {
    if (bowl_error === undefined) return 'muted';
    return bowl_error ? 'bad' : 'ok';
}

export function formatBatteryWire(wire) {
    if (wire === undefined) return '—';
    if (wire === 'unknown') return 'Unknown';
    return `${wire}%`;
}

export function formatMainsWire(wire) {
    if (wire === undefined) return '—';
    return wire === 'ON' ? 'Mains' : 'Battery';
}

export function formatBusy(busy) {
    return busy ? 'Feeding' : 'Idle';
}

export function busyTone(busy) {
    return busy ? 'warn' : 'ok';
}

export function statusStateRows(st) {
    const rows = [
        { label: 'Food in bowl', value: formatBowlWeightWire(st.bowl_weight), tone: bowlWeightTone(st.bowl_weight) },
        { label: 'Hopper', value: formatHopperLevel(st.hopper), tone: hopperTone(st.hopper) },
        { label: 'Activity', value: formatBusy(st.dispense_busy), tone: busyTone(st.dispense_busy) },
        { label: 'Bowl', value: formatBowlHealth(st.bowl_error), tone: bowlHealthTone(st.bowl_error) },
    ];
    if (st.battery !== undefined) {
        rows.push({ label: 'Battery', value: formatBatteryWire(st.battery), tone: st.battery === 'unknown' ? 'warn' : 'ok' });
    }
    if (st.mains !== undefined) {
        rows.push({ label: 'Power', value: formatMainsWire(st.mains), tone: st.mains === 'ON' ? 'ok' : 'muted' });
    }
    return rows;
}

export function formatDeviceTime(st) {
    return st.time_synced && st.local_time ? st.local_time : '—';
}

export function formatSlotClock(time) {
    const [h, m] = String(time).split(':');
    return `${String(h).padStart(2, '0')}:${String(m).padStart(2, '0')}`;
}

export function formatSlotSummary(time, g) {
    return `${formatSlotClock(time)} · ${g}g`;
}

export function weekdayFromLocalTime(local_time) {
    if (!local_time || local_time.length < 10) return null;
    const parts = local_time.slice(0, 10).split('-').map(Number);
    if (parts.length !== 3 || parts.some((n) => Number.isNaN(n))) return null;
    const sun0 = new Date(parts[0], parts[1] - 1, parts[2]).getDay();
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

export function dayDotClass(d) {
    let cls = 'day-dot';
    if (d.on) cls += ' on';
    if (d.today) cls += ' today';
    return cls;
}

export function slotCardMuted(slot) {
    return !slot?.enabled || slot?.state === 'to_be_skipped';
}

export function formatSlotState(state) {
    return state ? String(state).replace(/_/g, ' ') : '—';
}

export function slotStateTone(state) {
    return SLOT_TONE[state] || 'muted';
}

export function slotMinutesFromClock(time) {
    const parts = String(time).split(':');
    if (parts.length !== 2) return null;
    const h = Number(parts[0]);
    const m = Number(parts[1]);
    return Number.isNaN(h) || Number.isNaN(m) ? null : h * 60 + m;
}

export function slotIsPastToday(slot, local_time) {
    if (!slot?.today || !local_time) return false;
    const nowMin = slotMinutesFromClock(local_time.slice(11, 16));
    const slotMin = slotMinutesFromClock(slot.time);
    if (nowMin === null || slotMin === null) return false;
    return slotMin < nowMin;
}

export function slotSkipControl(slot, local_time) {
    if (slot?.enabled === false || !slot?.today || slotIsPastToday(slot, local_time) || SKIP_HIDE.has(slot.state)) return null;
    const active = slot.state === 'to_be_skipped';
    return {
        skip: !active,
        active,
        label: active ? 'Unskip today' : 'Skip today',
    };
}

export function parseApiResponse(text) {
    try {
        return JSON.parse(text);
    } catch {
        return text;
    }
}

export function apiContentType(body) {
    return body === undefined ? undefined : body.startsWith('{') ? 'application/json' : 'text/plain';
}

export function mutationMessage(r, okText, failPrefix) {
    if (r?.ok) return { text: okText, ok: true };
    const err = r?.error || 'unknown';
    return { text: failPrefix ? `${failPrefix}: ${err}` : err, ok: false };
}
