#include <string.h>
#include <time.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/i2c.h"

#include "lib/mfrc522.h"
#include "lib/pico_servo.h"

#include "inc/rtc_ntp.h"
#include "inc/aht10.h"
#include "inc/balanca.h"
#include "inc/setup.h"
#include "inc/mqtt_client.h"
#include "inc/controle.h"

#include "configura_geral.h"
#include "credenciais.h"

#include "FreeRTOS.h"
#include "task.h"

// Variável global do estado NTP
static NTP_T *ntp_state = NULL;

volatile bool wifi_conectado = false;
volatile bool mqtt_conectado = false;

TaskHandle_t ntp_task_handle;
TaskHandle_t aht10_task_handle;
TaskHandle_t controle_task_handle;
TaskHandle_t hx711_task_handle;


//
void task_wifi_mqtt(void *params) {
    if (wifi_connect() != 0) {
        printf("Falha na inicialização Wi-Fi. \n");
        vTaskDelete(NULL);
    }

    while (true) {
        printf("Tentando conectar ao Wi-Fi... \n");
        if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 10000) == 0) {
            printf("Wi-Fi conectado! \n");
            wifi_conectado = true;
            break;
        }
        printf("Falha na conexão Wi-Fi. Tentando novamente... \n");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }

    mqtt_do_connect();

    while (!mqtt_is_connected()) {
        printf("Aguardando conexão MQTT... \n");
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    printf("Conectado ao MQTT! \n");
    mqtt_conectado = true;

    // REABILIDA TAREFAS DEPENDENTES DO MQTT
    vTaskResume(ntp_task_handle);
    vTaskResume(aht10_task_handle);
    vTaskResume(controle_task_handle);
    vTaskResume(hx711_task_handle);

    while (true) {
        cyw43_arch_poll();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


// TAREFA -- SINSCRONIZAÇÃO NTP
void task_ntp_sync(void *params) {
    // COMEÇA SUSPENSA
    vTaskSuspend(NULL);

    ntp_state = ntp_init();
    if (!ntp_state) {
        printf("Falha ao inicializar NTP \n");
        vTaskDelete(NULL);
    }

    printf("Iniciando sincronização NTP... \n");
    start_ntp_request(ntp_state);

    while (true) {
        if (absolute_time_diff_us(get_absolute_time(), ntp_state->ntp_test_time) < 0) {
            start_ntp_request(ntp_state);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


// TAREFA --- LEITURA SENSOR AHT10 E PUBLICAÇÃO MQTT
void task_aht10(void *params) {
    // COMEÇA SUSPENSA
    vTaskSuspend(NULL);

    float temperatura, umidade;
    char payload_buffer[30];

    while (1) {
        aht10_trigger_measurement();

        if (aht10_read(&temperatura, &umidade)) {
            if (mqtt_is_connected()) {  // MELHOR NÃO TIRAR SENÃO O DIABO FICA ESTOURANDO O BUFFER
                sprintf(payload_buffer, "%.1f", temperatura);
                mqtt_do_publish(mqtt_get_client(), TOPICO_TEMPERATURA, payload_buffer, NULL);
                vTaskDelay(pdMS_TO_TICKS(500)); // tempo para enviar com segurança

                sprintf(payload_buffer, "%.1f", umidade);
                mqtt_do_publish(mqtt_get_client(), TOPICO_UMIDADE, payload_buffer, NULL);
                vTaskDelay(pdMS_TO_TICKS(500));
            }

            controle_ventilador(temperatura);
        }

        vTaskDelay(pdMS_TO_TICKS(3000)); // aguarda 3s entre leituras
    }
}


// TAREFA --- VERIFICAÇÃO DO TEMPO E CONTROLE AUTOMÁTICO DO SISTEMA
void task_controle_tempo(void *params) {
    // COMEÇA SUSPENSA
    vTaskSuspend(NULL);

    while (1) {
        rtc_loop_tick();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


// TAREFA --- LEITURA DA BALANÇA
void task_hx711(void *params) {
    // COMEÇA SUSPENSA
    vTaskSuspend(NULL);

    while (1) {
        coleta_peso();
    }
}


int main() {
    stdio_init_all();
    sleep_ms(2000);
    printf("Teste");

    // INICIALIZAÇÃO DOS PINOS E SENSORES
    //aht10_init();
    acionadores_setup();
    balancas_setup();
    rfid_init();
    //servo_init(RACAO_PIN);
    //servo_set_angle(RACAO_PIN, 180);

    // CRIA TAREFAS
    xTaskCreate(task_wifi_mqtt, "WiFi/MQTT", 4096, NULL, 1, NULL);
    xTaskCreate(task_ntp_sync, "NTP", 2048, NULL, 1, &ntp_task_handle);
    //xTaskCreate(task_aht10, "AHT10", 2048, NULL, 1, &aht10_task_handle);    // alterado, ver se nao vai causar erro
    xTaskCreate(task_controle_tempo, "Controle", 2048, NULL, 1, &controle_task_handle);
    xTaskCreate(task_hx711, "HX711", 2048, NULL, 1, &hx711_task_handle);

    // INICIA AGENDADOR
    vTaskStartScheduler();

    printf("Erro: scheduler não iniciado!\n");
    while (1) {
        tight_loop_contents();
    }

    return 0;
}