#include <string.h>

#include <pico/cyw43_arch.h>
#include <lwip/apps/mqtt.h>
#include <lwip/dns.h>

#include "mqtt_client.h"
#include "credenciais.h"
#include "configura_geral.h"
#include "inc/controle.h"

static mqtt_client_t *client = NULL;
bool mqtt_connected = false;

static char ultimo_topico[128] = {0};

// === Protótipos internos (static functions) ===
static void mqtt_dns_found_cb(const char *name, const ip_addr_t *ipaddr, void *arg);
static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status);
static void mqtt_pub_request_cb(void *arg, err_t result);
static void mqtt_sub_request_cb(void *arg, err_t result);
static void horarios_racao_callback(const char *payload, const char *momento);
static void horarios_limpeza_callback(const char *payload);
static void acionamento(const char *payload, int pino);
static void temperatura_atualizar(const char *payload);

int wifi_connect() {
    if (cyw43_arch_init_with_country(CYW43_COUNTRY_BRAZIL)) {
        printf("WiFi failed to initialise\n");
        return 1;
    }
    printf("WiFi initialised\n");

    cyw43_arch_enable_sta_mode();

    return 0;  // Só inicializa, não conecta aqui
}


static void mqtt_dns_found_cb(const char *name, const ip_addr_t *ipaddr, void *arg) {
    mqtt_client_t *client = (mqtt_client_t *)arg;

    if (ipaddr) {
        printf("DNS resolvido: %s\n", ipaddr_ntoa(ipaddr));

        struct mqtt_connect_client_info_t ci;
        memset(&ci, 0, sizeof(ci));
        ci.client_id = "pico";

        err_t err = mqtt_client_connect(client, ipaddr, 1883, mqtt_connection_cb, NULL, &ci);
        if (err != ERR_OK) {
            printf("Erro ao conectar: %d\n", err);
        }
    } else {
        printf("Erro na resolução DNS.\n");
    }
}

bool mqtt_is_connected() {
    return mqtt_connected;
}

mqtt_client_t *mqtt_get_client() {
    return client;
}

void mqtt_do_connect() {
    if (!client) {
        client = mqtt_client_new();
        if (!client) {
            printf("Erro ao criar cliente\n");
            return;
        }
    }

    ip_addr_t mqtt_ip;
    err_t err = dns_gethostbyname(HIVEMQ_HOST, &mqtt_ip, mqtt_dns_found_cb, client);
    if (err == ERR_OK) {
        mqtt_dns_found_cb(HIVEMQ_HOST, &mqtt_ip, client);
    } else if (err != ERR_INPROGRESS) {
        printf("Erro DNS imediato: %d\n", err);
    }
}

// Chamado quando uma nova publicação chega (metadados)
void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len) {
    strncpy(ultimo_topico, topic, sizeof(ultimo_topico) - 1);
    printf("Nova mensagem no tópico: %s (tamanho %d)\n", topic, tot_len);
}

// Chamado com o conteúdo da mensagem publicada
void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags) {
    char mensagem[128] = {0};

    char topico_local[128];
    strncpy(topico_local, ultimo_topico, sizeof(topico_local) - 1);
    topico_local[sizeof(topico_local) - 1] = '\0';

    if (len < sizeof(mensagem)) {
        memcpy(mensagem, data, len);
        mensagem[len] = '\0';
        printf("Mensagem recebida: %s\n", mensagem);

        // Agora você usa o último tópico armazenado
        if (strcmp(topico_local, TOPICO_RACAO_HORARIO_1) == 0) {
            horarios_racao_callback(mensagem, "racao_1");
        } 
        if (strcmp(topico_local, TOPICO_RACAO_HORARIO_2) == 0) {
            horarios_racao_callback(mensagem, "racao_2");
        } 
        if (strcmp(topico_local, TOPICO_RACAO_STATUS) == 0) {
            acionamento(mensagem, RACAO_PIN);
        }
        if (strcmp(topico_local, TOPICO_LIMPEZA_HORARIO) == 0) {
            horarios_limpeza_callback(mensagem);
        }
        if (strcmp(topico_local, TOPICO_LIMPEZA_STATUS) == 0) {
            acionamento(mensagem, LIMPEZA_PIN);
        }
        if (strcmp(topico_local, TOPICO_TEMPERATURA_LIMITE) == 0) {
            temperatura_atualizar(mensagem);
        }
    }
}

static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status) {
    if (status == MQTT_CONNECT_ACCEPTED) {
        printf("mqtt_connection_cb: Successfully connected\n");
        mqtt_connected = true;

        mqtt_set_inpub_callback(client, mqtt_incoming_publish_cb, mqtt_incoming_data_cb, client);

        mqtt_do_subscribe(client, TOPICO_RACAO_HORARIO_1);
        mqtt_do_subscribe(client, TOPICO_RACAO_HORARIO_2);
        mqtt_do_subscribe(client, TOPICO_RACAO_STATUS);
        mqtt_do_subscribe(client, TOPICO_LIMPEZA_HORARIO);
        mqtt_do_subscribe(client, TOPICO_LIMPEZA_STATUS);
        mqtt_do_subscribe(client, TOPICO_TEMPERATURA_LIMITE);

    } else {
        printf("mqtt_connection_cb: Disconnected, reason: %d\n", status);
        mqtt_connected = false;
        // Attempt to reconnect when connection fails
        mqtt_do_connect();
    }
}

void mqtt_do_publish(mqtt_client_t *client, char *topic, char *payload, void *arg) {
    if (!mqtt_connected) {
        printf("MQTT ainda não conectado. Ignorando publicação.\n");
        return;
    }

    err_t err;
    u8_t qos = MQTT_QOS; // 0 1 or 2, see MQTT specification
    u8_t retain = 0; // Don't want to retain payload
    err = mqtt_publish(client, topic, payload, strlen(payload), qos, retain, mqtt_pub_request_cb, arg);
    if (err != ERR_OK) {
        printf("Publish err: %d\n", err);
    }
}

// Called when publish is complete either with success or failure
static void mqtt_pub_request_cb(void *arg, err_t result) {
    if (result != ERR_OK) {
        printf("Publish result: %d\n", result);
    }
}

static void mqtt_sub_request_cb(void *arg, err_t result) {
    mqtt_client_t* client = (mqtt_client_t*)arg;
    if (result != ERR_OK) {
        panic("subscribe request failed %d", result);
    }
}

void mqtt_do_subscribe(mqtt_client_t *client, const char *topic) {
    if (!mqtt_connected) {
        printf("MQTT ainda não conectado. Não é possível assinar tópicos.\n");
        return;
    }

    err_t err = mqtt_subscribe(client, topic, MQTT_QOS, mqtt_sub_request_cb, client);
    if (err == ERR_OK) {
        printf("Assinado com sucesso: %s\n", topic);
    } else {
        printf("Erro ao assinar tópico: %s (código %d)\n", topic, err);
    }
}




static void horarios_racao_callback(const char *payload, const char *momento) {
    int hora, minuto;
    if (sscanf(payload, "%d:%d", &hora, &minuto) == 2) {
        atualizar_horario_racao(momento, hora, minuto);
    } else {
        printf("Formato inválido: %s\n", payload);
    }
}

static void horarios_limpeza_callback(const char *payload) {
    int hora, minuto;
    if (sscanf(payload, "%d:%d", &hora, &minuto) == 2) {
        atualizar_horario_limpeza(hora, minuto);
    } else {
        printf("Formato inválido: %s\n", payload);
    }
}


static void acionamento(const char *payload, int pino) {
    int comando;

    if (sscanf(payload, "%d", &comando) == 1 && comando == 1) {
        if (pino == RACAO_PIN) {
            acionar_racao_manual();
        } else if (pino == LIMPEZA_PIN) {
            acionar_limpeza_manual();
        }
    }
}

static void temperatura_atualizar(const char *payload) {
    float nova_temp;

    if (sscanf(payload, "%f", &nova_temp) == 1 && nova_temp > 0.0 && nova_temp < 100.0) {
        temp_maxima = nova_temp;
        printf("⚙️ Temperatura máxima para ventilador atualizada para: %.1f °C\n", temp_maxima);
    }
}


