#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include "lwip/apps/mqtt.h"

extern bool mqtt_connected;

// === Interface pública do módulo ===

// Conecta ao Wi-Fi
int wifi_connect(void);

// Verifica se está conectado ao MQTT
bool mqtt_is_connected(void);

// Inicializa a conexão MQTT (cria cliente + resolve DNS + conecta)
void mqtt_do_connect(void);

// Publica mensagem em um tópico
void mqtt_do_publish(mqtt_client_t *client, char *topic, char *payload, void *arg);

void mqtt_do_subscribe(mqtt_client_t *client, const char *topic);

// Retorna ponteiro do cliente MQTT (útil para publicar fora)
mqtt_client_t *mqtt_get_client(void);

#endif