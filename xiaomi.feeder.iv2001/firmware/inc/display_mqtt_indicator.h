/*
 * MQTT status lightbar policy — spec/30-processes/display-presentation.md § MQTT indicator
 */

#ifndef DISPLAY_MQTT_INDICATOR_H
#define DISPLAY_MQTT_INDICATOR_H

void display_mqtt_indicator_connecting(void);
void display_mqtt_indicator_connected(void);
void display_mqtt_indicator_error(void);
void display_mqtt_indicator_off(void);

#endif /* DISPLAY_MQTT_INDICATOR_H */
