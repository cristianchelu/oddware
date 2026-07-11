import assert from 'node:assert/strict';
import { describe, it } from 'node:test';

import {
    DAYS,
    ALL_DAYS_MASK,
    apiContentType,
    buildDispenseBody,
    buildFeedOverfillBody,
    buildScheduleSetBody,
    buildScheduleToggleBody,
    busyTone,
    dayDotClass,
    formatBusy,
    formatDeviceTime,
    formatHopperLevel,
    formatNextFeedParts,
    formatSlotClock,
    formatSlotSummary,
    weekdayFromLocalTime,
    formatSlotState,
    formatStatusAlerts,
    hopperTone,
    indicesToMask,
    maskToIndices,
    mutationMessage,
    parseApiResponse,
    readMaskFromCheckedValues,
    slotStateTone,
    slotDayBadges,
    slotSkipControl,
    slotCardMuted,
    slotIsPastToday,
    sortSlotsByTime,
    statusStateRows,
    validateScheduleMask,
} from './logic.mjs';

describe('weekday mask', () => {
    it('Mon only is index 0 and mask 1', () => {
        assert.deepEqual(maskToIndices(1), [0]);
        assert.equal(indicesToMask([0]), 1);
    });

    it('all days is mask 127', () => {
        assert.deepEqual(maskToIndices(127), [0, 1, 2, 3, 4, 5, 6]);
        assert.equal(indicesToMask([0, 1, 2, 3, 4, 5, 6]), 127);
    });

    it('reads checkbox values', () => {
        assert.equal(readMaskFromCheckedValues(['0', '2']), 5);
    });
});

describe('validateScheduleMask', () => {
    it('rejects empty mask', () => {
        const r = validateScheduleMask(0);
        assert.equal(r.ok, false);
        assert.equal(r.error, 'Pick at least one day');
    });

    it('accepts non-zero mask', () => {
        assert.equal(validateScheduleMask(1).ok, true);
    });
});

describe('buildScheduleSetBody', () => {
    it('matches web_api schedule set wire format', () => {
        const body = buildScheduleSetBody({
            hour: 7,
            min: 15,
            mask: 127,
            g: 40,
            enabled: true,
        });
        assert.deepEqual(body, {
            hour: 7,
            min: 15,
            repeat_days: [0, 1, 2, 3, 4, 5, 6],
            g: 40,
            enabled: true,
        });
    });
});

describe('formatters', () => {
    it('formatNextFeedParts', () => {
        assert.deepEqual(formatNextFeedParts({ hour: 7, min: 0, g: 30, in_min: 47 }), {
            headline: 'Next feed · in 47 min',
            detail: '07:00 · 30g',
        });
        assert.deepEqual(formatNextFeedParts({ hour: 7, min: 5, g: 40, in_min: 0 }), {
            headline: 'Next feed · now',
            detail: '07:05 · 40g',
        });
        assert.deepEqual(formatNextFeedParts(null), {
            headline: 'Next feed · nothing scheduled',
            detail: null,
        });
    });

    it('sortSlotsByTime', () => {
        const sorted = sortSlotsByTime([
            { time: '18:00' },
            { time: '07:15' },
            { time: '07:05' },
        ]);
        assert.deepEqual(sorted.map((s) => s.time), ['07:05', '07:15', '18:00']);
    });

    it('ALL_DAYS_MASK', () => {
        assert.equal(ALL_DAYS_MASK, 127);
    });

    it('formatDeviceTime', () => {
        assert.equal(
            formatDeviceTime({ time_synced: true, local_time: '2026-07-05 19:00' }),
            '2026-07-05 19:00',
        );
        assert.equal(formatDeviceTime({ time_synced: false }), '—');
        assert.equal(formatDeviceTime({ time_synced: true }), '—');
    });

    it('formatStatusAlerts', () => {
        assert.deepEqual(
            formatStatusAlerts({ time_synced: true, hopper: 'ok', dispense_busy: false }),
            [],
        );
        const alerts = formatStatusAlerts({
            time_synced: false,
            bowl_error: true,
        });
        assert.equal(alerts.length, 2);
        assert.equal(alerts[0].text, 'Time not synced');
        assert.equal(alerts[1].text, 'Bowl problem');
    });

    it('statusStateRows', () => {
        const rows = statusStateRows({
            bowl_weight: '42.3',
            hopper: 'low',
            dispense_busy: true,
            bowl_error: false,
            battery: '85',
            mains: 'ON',
        });
        assert.equal(rows[0].value, '42.3 g');
        assert.equal(rows[1].value, 'Low');
        assert.equal(rows[1].tone, 'warn');
        assert.equal(rows[2].value, 'Feeding');
        assert.equal(rows[4].value, '85%');
        assert.equal(rows[5].value, 'Mains');
    });

    it('buildScheduleToggleBody', () => {
        assert.deepEqual(buildScheduleToggleBody(7, 15), { hour: 7, min: 15 });
    });

    it('formatHopperLevel and hopperTone', () => {
        assert.equal(formatHopperLevel('empty'), 'Empty');
        assert.equal(hopperTone('empty'), 'bad');
        assert.equal(formatHopperLevel('low'), 'Low');
        assert.equal(hopperTone('normal'), 'ok');
    });

    it('formatBusy and busyTone', () => {
        assert.equal(formatBusy(true), 'Feeding');
        assert.equal(busyTone(true), 'warn');
        assert.equal(formatBusy(false), 'Idle');
    });

    it('dayDotClass', () => {
        assert.equal(dayDotClass({ on: false, today: false }), 'day-dot');
        assert.equal(dayDotClass({ on: true, today: false }), 'day-dot on');
        assert.equal(dayDotClass({ on: false, today: true }), 'day-dot today');
        assert.equal(dayDotClass({ on: true, today: true }), 'day-dot on today');
    });

    it('slotDayBadges', () => {
        const badges = slotDayBadges([0, 2, 4]);
        assert.equal(badges.length, 7);
        assert.deepEqual(
            badges.map((b) => (b.on ? b.letter : '.')),
            ['M', '.', 'W', '.', 'F', '.', '.'],
        );
        assert.equal(badges[0].label, 'Mon');
        assert.equal(badges[6].on, false);
        const wed = slotDayBadges([0, 2, 4], 2)[2];
        assert.equal(wed.today, true);
        assert.equal(wed.on, true);
    });

    it('formatSlotClock and formatSlotSummary', () => {
        assert.equal(formatSlotClock('7:0'), '07:00');
        assert.equal(formatSlotClock('18:30'), '18:30');
        assert.equal(formatSlotSummary('7:15', 40), '07:15 · 40g');
    });

    it('weekdayFromLocalTime', () => {
        assert.equal(weekdayFromLocalTime('2026-07-06 08:00:00'), 0);
        assert.equal(weekdayFromLocalTime(null), null);
    });

    it('formatSlotState and slotStateTone', () => {
        assert.equal(formatSlotState('dispensed'), 'dispensed');
        assert.equal(formatSlotState('to_be_skipped'), 'to be skipped');
        assert.equal(slotStateTone('dispensed'), 'ok');
        assert.equal(slotStateTone('to_be_skipped'), 'warn');
        assert.equal(slotStateTone('missed'), 'bad');
    });

    it('slotSkipControl', () => {
        const local = '2026-07-09 10:00:00';
        const past = { time: '7:00', today: true, state: 'skipped' };
        const futurePending = { time: '18:30', today: true, state: 'pending' };
        const futureSkipped = { time: '18:30', today: true, state: 'to_be_skipped' };
        assert.equal(slotSkipControl(past, local), null);
        assert.equal(slotSkipControl({ ...futurePending, enabled: false }, local), null);
        assert.deepEqual(slotSkipControl(futurePending, local), {
            skip: true,
            active: false,
            label: 'Skip today',
        });
        assert.deepEqual(slotSkipControl(futureSkipped, local), {
            skip: false,
            active: true,
            label: 'Unskip today',
        });
        assert.equal(slotIsPastToday(past, local), true);
        assert.equal(slotIsPastToday(futurePending, local), false);
    });

    it('slotCardMuted', () => {
        assert.equal(slotCardMuted({ enabled: true, state: 'pending' }), false);
        assert.equal(slotCardMuted({ enabled: false, state: 'pending' }), true);
        assert.equal(slotCardMuted({ enabled: true, state: 'to_be_skipped' }), true);
    });
});

describe('api helpers', () => {
    it('parseApiResponse json', () => {
        assert.deepEqual(parseApiResponse('{"ok":true}'), { ok: true });
    });

    it('parseApiResponse plain text', () => {
        assert.equal(parseApiResponse('compensated'), 'compensated');
    });

    it('apiContentType', () => {
        assert.equal(apiContentType('{"g":30}'), 'application/json');
        assert.equal(apiContentType('open_loop'), 'text/plain');
    });

    it('mutationMessage', () => {
        assert.deepEqual(mutationMessage({ ok: true }, 'saved'), { text: 'saved', ok: true });
        assert.deepEqual(mutationMessage({ ok: false, error: 'busy' }, 'saved'), {
            text: 'busy',
            ok: false,
        });
        assert.deepEqual(mutationMessage({ ok: false, error: 'busy' }, 'saved', 'fail'), {
            text: 'fail: busy',
            ok: false,
        });
    });

    it('buildDispenseBody', () => {
        assert.deepEqual(buildDispenseBody('30'), { g: 30 });
    });

    it('buildFeedOverfillBody', () => {
        assert.deepEqual(buildFeedOverfillBody({ enabled: true, threshold_g: '40' }), {
            enabled: true,
            threshold_g: 40,
        });
        assert.deepEqual(buildFeedOverfillBody({ enabled: false }), { enabled: false });
    });
});
