import assert from 'node:assert/strict';
import { describe, it } from 'node:test';

import { effectiveSlotState } from './mock_api.mjs';

describe('mock_api effectiveSlotState', () => {
    const sched = {
        time_synced: true,
        local_time: '2026-07-09 10:00:00',
        today_enabled: true,
        schedule_enabled: true,
    };

    it('past-due today becomes skipped', () => {
        const slot = {
            hour: 7,
            min: 0,
            repeat_days: [0, 1, 2, 3, 4],
            enabled: true,
            skip_today: false,
            terminal: null,
        };
        assert.equal(effectiveSlotState(slot, sched), 'skipped');
    });

    it('future today stays pending', () => {
        const slot = {
            hour: 18,
            min: 30,
            repeat_days: [0, 1, 2, 3, 4, 5, 6],
            enabled: true,
            skip_today: false,
            terminal: null,
        };
        assert.equal(effectiveSlotState(slot, sched), 'pending');
    });

    it('preserves terminal outcomes', () => {
        const slot = {
            hour: 7,
            min: 0,
            repeat_days: [0, 1, 2, 3, 4],
            enabled: true,
            skip_today: false,
            terminal: 'dispensed',
        };
        assert.equal(effectiveSlotState(slot, sched), 'dispensed');
    });

    it('skip_today future vs past', () => {
        const base = {
            hour: 7,
            min: 0,
            repeat_days: [0, 1, 2, 3, 4],
            enabled: true,
            skip_today: true,
            terminal: null,
        };
        assert.equal(effectiveSlotState(base, sched), 'skipped');
        const future = { ...base, hour: 18, min: 30, repeat_days: [0, 1, 2, 3, 4, 5, 6] };
        assert.equal(effectiveSlotState(future, sched), 'to_be_skipped');
    });

    it('disabled past slot stays pending', () => {
        const slot = {
            hour: 7,
            min: 0,
            repeat_days: [0, 1, 2, 3, 4],
            enabled: false,
            skip_today: false,
            terminal: null,
        };
        assert.equal(effectiveSlotState(slot, sched), 'pending');
    });
});
