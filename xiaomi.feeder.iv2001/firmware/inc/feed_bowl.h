/*
 * Bowl mass helpers for feed policy — spec/30-processes/scheduler-engine.md § Overfill
 */

#ifndef FEED_BOWL_H
#define FEED_BOWL_H

#include <stdbool.h>
#include <stdint.h>

#include "schedule.h"

bool feed_bowl_known_g(uint32_t now_ms, uint16_t *grams_out);
bool feed_overfill_should_skip_schedule(uint32_t now_ms);
schedule_fire_result_t feed_schedule_fire(uint8_t g, uint32_t now_ms);

#endif /* FEED_BOWL_H */
