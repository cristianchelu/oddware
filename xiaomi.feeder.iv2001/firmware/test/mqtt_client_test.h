#ifndef MQTT_CLIENT_TEST_H
#define MQTT_CLIENT_TEST_H

#include <stdint.h>

void mqtt_client_test_reset(void);
void mqtt_client_test_bootstrap(void);
uint32_t mqtt_client_step(void);

#endif /* MQTT_CLIENT_TEST_H */
