import assert from 'node:assert/strict';
import { describe, it } from 'node:test';

import {
    DAYS,
    apiContentType,
    buildDispenseBody,
    buildScheduleSetBody,
    buildScheduleToggleBody,
    busyTone,
    formatBusy,
    formatDeviceTime,
    formatFeedModeLabel,
    formatHopper,
    formatMetaLine,
    formatNextFeed,
    formatNextFeedParts,
    formatRefreshTime,
    formatSlotClock,
    formatSlotDayLetters,
    formatSlotDays,
    weekdayFromLocalTime,
    formatSlotState,
    formatStatusAlerts,
    formatStatusMessage,
    hopperTone,
    indicesToMask,
    maskToIndices,
    mutationMessage,
    parseApiResponse,
    readMaskFromCheckedValues,
    slotStateTone,
    slotDayBadges,
    sortSlotsByTime,
    statusStateRows,
    validateScheduleMask,
    WEEKDAY_MASK,
    ALL_DAYS_MASK,
    formatEditorTitle,
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
    it('formatNextFeed with next object', () => {
        const line = formatNextFeed({ hour: 7, min: 5, g: 40, in_min: 12 });
        assert.equal(line, '7:05 · 40g · 12m');
    });

    it('formatNextFeed without next', () => {
        assert.equal(formatNextFeed(null), 'No upcoming feed');
    });

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

    it('formatEditorTitle', () => {
        assert.equal(formatEditorTitle(null), 'New feeding time');
        assert.equal(formatEditorTitle('07:15'), 'Edit 07:15');
    });

    it('day presets', () => {
        assert.equal(WEEKDAY_MASK, 31);
        assert.equal(ALL_DAYS_MASK, 127);
    });

    it('formatStatusMessage', () => {
        const line = formatStatusMessage({
            time_synced: true,
            local_time: '2026-07-05 19:00',
            hopper: 'ok',
            dispense_busy: false,
        });
        assert.equal(line, '2026-07-05 19:00 | hopper ok | busy false');
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

    it('formatRefreshTime and formatMetaLine', () => {
        const at = new Date('2026-07-05T19:42:00');
        assert.match(formatRefreshTime(at), /19:42/);
        const line = formatMetaLine(
            { hopper: 'normal', dispense_busy: false },
            at,
        );
        assert.match(line, /hopper ok · idle/);
    });

    it('buildScheduleToggleBody', () => {
        assert.deepEqual(buildScheduleToggleBody(7, 15), { hour: 7, min: 15 });
    });

    it('formatHopper and hopperTone', () => {
        assert.equal(formatHopper('empty'), 'Empty');
        assert.equal(hopperTone('empty'), 'bad');
        assert.equal(formatHopper('low'), 'Low');
        assert.equal(hopperTone('normal'), 'ok');
    });

    it('formatBusy and busyTone', () => {
        assert.equal(formatBusy(true), 'Feeding');
        assert.equal(busyTone(true), 'warn');
        assert.equal(formatBusy(false), 'Idle');
    });

    it('formatFeedModeLabel', () => {
        assert.equal(formatFeedModeLabel('compensated'), 'Compensated');
        assert.equal(formatFeedModeLabel('open_loop'), 'Open loop');
    });

    it('formatSlotDayLetters and formatSlotDays', () => {
        assert.equal(formatSlotDayLetters([0, 2, 4]), 'MWF');
        assert.equal(formatSlotDays([0, 2, 4]), 'Mon, Wed, Fri');
        assert.equal(DAYS.length, 7);
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

    it('formatSlotClock and weekdayFromLocalTime', () => {
        assert.equal(formatSlotClock('7:0'), '07:00');
        assert.equal(formatSlotClock('18:30'), '18:30');
        assert.equal(weekdayFromLocalTime('2026-07-06 08:00:00'), 0);
        assert.equal(weekdayFromLocalTime(null), null);
    });

    it('formatSlotState and slotStateTone', () => {
        assert.equal(formatSlotState('dispensed'), 'dispensed');
        assert.equal(slotStateTone('dispensed'), 'ok');
        assert.equal(slotStateTone('missed'), 'bad');
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
});
