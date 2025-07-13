#include <string.h>

#include <pico/cyw43_arch.h>
#include <lwip/apps/mqtt.h>
#include <lwip/dns.h>

#include "inc/controle.h"
#include "mqtt_client.h"
#include "credenciais.h"
#include "configura_geral.h"

// --- VARIÁVEIS MQTT
static mqtt_client_t *client = NULL;
bool mqtt_connected = false;

// 
static char ultimo_topico[128] = {0};

// PROTÓTIPOS DAS FUNÇÕES
static void mqtt_dns_found_cb(const char *name, const ip_addr_t *ipaddr, void *arg);
static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status);
static void mqtt_pub_request_cb(void *arg, err_t result);
static void mqtt_sub_request_cb(void *arg, err_t result);
static void horarios_racao_callback(const char *payload, const char *momento);
static void horarios_limpeza_callback(const char *payload);
static void acionamento(const char *payload, int pino);
static void temperatura_atualizar(const char *payload);

// --- FUNÇÕES DO CLIENTE MQTT
// CONEXÃO WIFI
int wifi_connect() {
    // VERIFICA DE INICIALIZAÇÃO FOI CONCLUÍDA
    if (cyw43_arch_init_with_country(CYW43_COUNTRY_BRAZIL)) {
        printf("Falha na inicialização do WiFi \n");
        return 1;
    }
    printf("WiFi inicializado \n");

    // HABILITA STATION MODE
    cyw43_arch_enable_sta_mode();

    return 0;
}

// DNS 
static void mqtt_dns_found_cb(const char *name, const ip_addr_t *ipaddr, void *arg) {
    mqtt_client_t *client = (mqtt_client_t *)arg;

    if (ipaddr) {
        printf("DNS resolvido: %s \n", ipaddr_ntoa(ipaddr));

        struct mqtt_connect_client_info_t ci;
        memset(&ci, 0, sizeof(ci));
        ci.client_id = MQTT_CLIENT_ID;  // se não funcionar, voltar pra pico

        err_t err = mqtt_client_connect(client, ipaddr, 1883, mqtt_connection_cb, NULL, &ci);
        if (err != ERR_OK) {
            printf("Erro ao conectar: %d \n", err);
        }
    } else {
        printf("Erro na resolução DNS \n");
    }
}

// FUNÇÃO DE RETORNO DE STATUS DE CONEXÃO MQTT
bool mqtt_is_connected() {
    return mqtt_connected;
}

// FUNÇÃOD DE RETORNO DE CLIENTE MQTT 
mqtt_client_t *mqtt_get_client() {
    return client;
}

// CONEXÃO MQTT
void mqtt_do_connect() {
    // CRIAR CLIENTE SE NÃO HOUVER UM
    if (!client) {
        client = mqtt_client_new();
        if (!client) {
            printf("Erro ao criar cliente \n");
            return;
        }
    }

    // INICIA RESOLUÇÃO DNS PARA BROKER
    ip_addr_t mqtt_ip;
    err_t err = dns_gethostbyname(HIVEMQ_HOST, &mqtt_ip, mqtt_dns_found_cb, client);
    if (err == ERR_OK) {
        mqtt_dns_found_cb(HIVEMQ_HOST, &mqtt_ip, client);
    } else if (err != ERR_INPROGRESS) {
        printf("Erro DNS imediato: %d \n", err);
    }
}

// CALLBACK QUANDO UMA NOVA PUBLICAÇÃO CHEGA
void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len) {
    strncpy(ultimo_topico, topic, sizeof(ultimo_topico) - 1);
    printf("Nova mensagem no tópico: %s (tamanho %d) \n", topic, tot_len);
}

// CALLBACK COM CONTEÚDO DA MENSAGEM PUBLICADA 
void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags) {
    char mensagem[128] = {0};

    char topico_local[128];
    strncpy(topico_local, ultimo_topico, sizeof(topico_local) - 1);
    topico_local[sizeof(topico_local) - 1] = '\0';

    if (len < sizeof(mensagem)) {
        memcpy(mensagem, data, len);
        mensagem[len] = '\0';
        printf("Mensagem recebida: %s \n", mensagem);

        // CHAMA FUNÇÕES PARA MENSAGEM RESPECTIVA RECEBIDA
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

//
static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status) {
    if (status == MQTT_CONNECT_ACCEPTED) {
        printf("Mqtt conectado com sucesso \n");
        mqtt_connected = true;

        //
        mqtt_set_inpub_callback(client, mqtt_incoming_publish_cb, mqtt_incoming_data_cb, client);

        // ASSINA TODOS OS TÓPICOS
        mqtt_do_subscribe(client, TOPICO_RACAO_HORARIO_1);
        mqtt_do_subscribe(client, TOPICO_RACAO_HORARIO_2);
        mqtt_do_subscribe(client, TOPICO_RACAO_STATUS);
        mqtt_do_subscribe(client, TOPICO_LIMPEZA_HORARIO);
        mqtt_do_subscribe(client, TOPICO_LIMPEZA_STATUS);
        mqtt_do_subscribe(client, TOPICO_TEMPERATURA_LIMITE);

    } else {
        printf("Mqtt desconectado, motivo: %d\n", status);
        mqtt_connected = false;
        // TENTA RECONECTAR
        mqtt_do_connect();
    }
}

// FUNÇÃO DE PUBLICAÇÃO DOS TÓPICOS
void mqtt_do_publish(mqtt_client_t *client, char *topic, char *payload, void *arg) {
    if (!mqtt_connected) {
        printf("MQTT ainda não conectado, ignorando publicação \n");
        return;
    }

    err_t err;
    u8_t qos = MQTT_QOS; 
    u8_t retain = 0;
    err = mqtt_publish(client, topic, payload, strlen(payload), qos, retain, mqtt_pub_request_cb, arg);
    if (err != ERR_OK) {
        printf("Erro de publicação: %d\n", err);
    }
}

// CALLBACK QUANDO PUBLICAÇÃO É COMPLETADA
static void mqtt_pub_request_cb(void *arg, err_t result) {
    if (result != ERR_OK) {
        printf("Resultado publicação: %d\n", result);
    }
}

// SOLICITAÇÃO DE ASSINATURA
static void mqtt_sub_request_cb(void *arg, err_t result) {
    mqtt_client_t* client = (mqtt_client_t*)arg;
    if (result != ERR_OK) {
        panic("Falha na solicitação de assinatura %d", result);
    }
}

// FUNÇÃO DE ASSINATURA DOS TÓPICOS
void mqtt_do_subscribe(mqtt_client_t *client, const char *topic) {
    if (!mqtt_connected) {
        printf("MQTT ainda não conectado, não é possível assinar tópicos.\n");
        return;
    }

    err_t err = mqtt_subscribe(client, topic, MQTT_QOS, mqtt_sub_request_cb, client);
    if (err == ERR_OK) {
        printf("Assinado com sucesso: %s\n", topic);
    } else {
        printf("Erro ao assinar tópico: %s (código %d)\n", topic, err);
    }
}


// --- FUNÇÕES PARA ASSINATURAS

// VERIFICA MENSAGEM RECEBIDA E ATUALIZA HORÁRIOS DA RAÇÃO
static void horarios_racao_callback(const char *payload, const char *momento) {
    int hora, minuto;
    if (sscanf(payload, "%d:%d", &hora, &minuto) == 2) {
        atualizar_horario_racao(momento, hora, minuto);
    } else {
        printf("Formato inválido: %s \n", payload);
    }
}

// VERIFICA MENSAGEM RECEBIDA E ATUALIZA HORÁRIOS DA LIMPEZA
static void horarios_limpeza_callback(const char *payload) {
    int hora, minuto;
    if (sscanf(payload, "%d:%d", &hora, &minuto) == 2) {
        atualizar_horario_limpeza(hora, minuto);
    } else {
        printf("Formato inválido: %s\n", payload);
    }
}

// VERIFICA MENSAGEM RECEBIDA E ACIONA RAÇÃO OU LIMPEZA
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

// VERIFICA MENSAGEM RECEBIDA E ATUALIZA TEMPERATURA
static void temperatura_atualizar(const char *payload) {
    float nova_temp;

    if (sscanf(payload, "%f", &nova_temp) == 1 && nova_temp > 0.0 && nova_temp < 100.0) {
        temp_maxima = nova_temp;
        printf("Temperatura máxima para ventilador atualizada para: %.1f °C \n", temp_maxima);
    }
}


