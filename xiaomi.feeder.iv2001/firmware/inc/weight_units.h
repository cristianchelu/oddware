/*
 * Weight unit helpers — spec/30-processes/weighing.md § Data model
 */

#ifndef WEIGHT_UNITS_H
#define WEIGHT_UNITS_H

#include <stddef.h>
#include <stdint.h>

typedef int32_t weight_dg_t;

#define WEIGHT_G_TO_DG(g)          ((weight_dg_t)(g) * 10)
#define WEIGHT_DG_TO_G_ROUND(dg)   (((dg) + ((dg) >= 0 ? 5 : -5)) / 10)
#define WEIGHT_DISPLAY_MAX_G       999u

uint16_t weight_dg_to_display_g(weight_dg_t dg);
int weight_format_mqtt_g(weight_dg_t dg, char *buf, size_t len);
int weight_format_cli_g(weight_dg_t dg, char *buf, size_t len);

#endif /* WEIGHT_UNITS_H */
