#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include "lwip/apps/mqtt.h"

extern bool mqtt_connected;

int wifi_connect(void);

bool mqtt_is_connected(void);
void mqtt_do_connect(void);

void mqtt_do_publish(mqtt_client_t *client, char *topic, char *payload, void *arg);
void mqtt_do_subscribe(mqtt_client_t *client, const char *topic);

mqtt_client_t *mqtt_get_client(void);

#endif