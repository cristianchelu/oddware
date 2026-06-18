/*
 * Battery percentage from pack voltage — spec/30-processes/battery-monitoring.md
 */

#include "battery_pct.h"

typedef struct {
    uint16_t mv;
    uint8_t pct;
} battery_pct_knot_t;

static const battery_pct_knot_t s_aa_alk_4s_knots[] = {
    { 6000u, 100u },
    { 5600u,  75u },
    { 5200u,  50u },
    { 4800u,  25u },
    { 4400u,  10u },
    { 4000u,   0u },
};

#define AA_ALK_4S_KNOT_COUNT  (uint8_t)(sizeof(s_aa_alk_4s_knots) / sizeof(s_aa_alk_4s_knots[0]))

static battery_chemistry_t battery_pct_normalize_chemistry(battery_chemistry_t chem)
{
    if (chem >= BATTERY_CHEM_COUNT) {
        return BATTERY_CHEM_AA_ALK_4S;
    }

    return chem;
}

static uint8_t battery_pct_interpolate(const battery_pct_knot_t *knots,
                                       uint8_t knot_count,
                                       uint16_t pack_mv)
{
    uint8_t i;

    if (knot_count == 0u) {
        return 0u;
    }

    if (pack_mv >= knots[0].mv) {
        return knots[0].pct;
    }

    if (pack_mv <= knots[knot_count - 1u].mv) {
        return knots[knot_count - 1u].pct;
    }

    for (i = 0u; i < (knot_count - 1u); i++) {
        uint16_t mv_hi = knots[i].mv;
        uint16_t mv_lo = knots[i + 1u].mv;
        uint8_t pct_hi = knots[i].pct;
        uint8_t pct_lo = knots[i + 1u].pct;
        uint32_t span_mv;
        uint32_t offset_mv;
        uint32_t delta_pct;

        if (pack_mv > mv_hi || pack_mv < mv_lo) {
            continue;
        }

        span_mv = (uint32_t)mv_hi - (uint32_t)mv_lo;
        offset_mv = (uint32_t)pack_mv - (uint32_t)mv_lo;
        delta_pct = (uint32_t)pct_hi - (uint32_t)pct_lo;

        return (uint8_t)(pct_lo + (offset_mv * delta_pct + span_mv / 2u) / span_mv);
    }

    return 0u;
}

battery_chemistry_t battery_pct_default_chemistry(void)
{
    return BATTERY_CHEM_AA_ALK_4S;
}

uint8_t battery_pct_from_mv(uint16_t pack_mv, battery_chemistry_t chem)
{
    chem = battery_pct_normalize_chemistry(chem);

    switch (chem) {
    case BATTERY_CHEM_AA_ALK_4S:
        return battery_pct_interpolate(s_aa_alk_4s_knots,
                                       AA_ALK_4S_KNOT_COUNT,
                                       pack_mv);
    default:
        return battery_pct_interpolate(s_aa_alk_4s_knots,
                                       AA_ALK_4S_KNOT_COUNT,
                                       pack_mv);
    }
}
