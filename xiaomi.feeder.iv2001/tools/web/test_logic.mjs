import assert from 'node:assert/strict';
import { describe, it } from 'node:test';

import {
    DAYS,
    apiContentType,
    buildDispenseBody,
    buildScheduleSetBody,
    formatNextFeed,
    formatSlotDayLetters,
    formatStatusMessage,
    indicesToMask,
    maskToIndices,
    mutationMessage,
    parseApiResponse,
    readMaskFromCheckedValues,
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
    it('formatNextFeed with next object', () => {
        const line = formatNextFeed({ hour: 7, min: 5, g: 40, in_min: 12 });
        assert.equal(line, 'Next: 7:05 40g in 12m');
    });

    it('formatNextFeed without next', () => {
        assert.equal(formatNextFeed(null), 'No upcoming feed');
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

    it('formatSlotDayLetters', () => {
        assert.equal(formatSlotDayLetters([0, 2, 4]), 'MWF');
        assert.equal(DAYS.length, 7);
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
