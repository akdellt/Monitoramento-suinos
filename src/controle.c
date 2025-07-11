#include "controle.h"

#include "hardware/gpio.h"
#include "hardware/rtc.h"
#include "pico/util/datetime.h"
#include "configura_geral.h"
#include "lib/pico_servo.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "pico/stdlib.h"

// MUDAR ACIONADOR DE RAÇÃO PARA SERVO MOTOR

// === Variáveis de horário programado ===
int hora_racao_1 = 6, hora_racao_2 = 18;
int minuto_racao_1 = 0, minuto_racao_2 = 0;

int hora_limpeza = 12;
int minuto_limpeza = 0;

// === Flags de controle diário ===
static bool racao1_acionada_hoje, racao2_acionada_hoje = false;
static bool limpeza_acionada_hoje = false;

static bool racao_acionada_manual = false;
static bool limpeza_acionada_manual = false;

// === Variáveis para controle do tempo de acionamento (não bloqueante) ===
static bool racao_em_execucao = false;
static absolute_time_t racao_acionamento_inicio;

static bool limpeza_em_execucao = false;
static absolute_time_t limpeza_acionamento_inicio;

float temp_maxima = 30.f;
static bool ventilador_ligado = false;

void controle_ventilador(float temperatura) {
    if (temperatura >= temp_maxima && !ventilador_ligado) {
        gpio_put(VENTILADOR_PIN, 1);
        ventilador_ligado = true;
        printf("[AHT10] 🚨 Temperatura alta (%.1f°C). Ventilador LIGADO.\n", temperatura);
    } else if (temperatura <= (temp_maxima - 2) && ventilador_ligado) {
        gpio_put(VENTILADOR_PIN, 0);
        ventilador_ligado = false;
        printf("[AHT10] ✅ Temperatura normalizada (%.1f°C). Ventilador DESLIGADO.\n", temperatura);
    }
}

void atualizar_horario_racao(const char *momento, int hora, int minuto) {
    if (strcmp(momento, "racao_1") == 0) {
        hora_racao_1 = hora;
        minuto_racao_1 = minuto;
    }
    else if (strcmp(momento, "racao_2") == 0) {
        hora_racao_2 = hora;
        minuto_racao_2 = minuto;
    }
    
    printf("⏰ Novo horário da ração: %02d:%02d\n", hora, minuto);
}

void atualizar_horario_limpeza(int hora, int minuto) {
    hora_limpeza = hora;
    minuto_limpeza = minuto;
    printf("⏰ Novo horário da limpeza: %02d:%02d\n", hora, minuto);
}

// MUDAR ACIONADOR DE RAÇÃO PARA SERVO MOTOR
void acionar_racao_manual() {
    if (!racao_em_execucao) {
        printf("\n[MQTT] Acionando dispenser de ração!\n");
        servo_set_angle(RACAO_PIN, 90);
        //gpio_put(RACAO_PIN, 1);
        racao_acionamento_inicio = get_absolute_time();
        racao_em_execucao = true;
        racao_acionada_manual = true;
    } else {
        printf("[MQTT] ⚠️ Ração já em execução.\n");
    }
}

void acionar_limpeza_manual() {
    if (!limpeza_em_execucao) {
        printf("\n[MQTT] Acionando irrigadores de limpeza!\n");
        gpio_put(LIMPEZA_PIN, 1);
        limpeza_acionamento_inicio = get_absolute_time();
        limpeza_em_execucao = true;
        limpeza_acionada_manual = true;
    } else {
        printf("[MQTT] ⚠️ Limpeza já em execução.\n");
    }
}

// Verifica se é hora de acionar a ração e gerencia desligamento após 5s
void verificar_acionamento_racao(const datetime_t *t) {
    if (!rtc_running()) return;

    // Acionamento automático programado: horário 1
    if (t->hour == hora_racao_1 && t->min == minuto_racao_1 && !racao1_acionada_hoje && !racao_em_execucao) {
        printf("\n[RTC] HORA DA PRIMEIRA RAÇÃO! Acionando dispenser.\n");
        servo_set_angle(RACAO_PIN, 90);
        //gpio_put(RACAO_PIN, 1);
        racao_acionamento_inicio = get_absolute_time();
        racao_em_execucao = true;
        racao1_acionada_hoje = true;
    }

    // Acionamento automático programado: horário 2
    if (t->hour == hora_racao_2 && t->min == minuto_racao_2 && !racao2_acionada_hoje && !racao_em_execucao) {
        printf("\n[RTC] HORA DA SEGUNDA RAÇÃO! Acionando dispenser.\n");
        servo_set_angle(RACAO_PIN, 90);
        //gpio_put(RACAO_PIN, 1);
        racao_acionamento_inicio = get_absolute_time();
        racao_em_execucao = true;
        racao2_acionada_hoje = true;
    }

    // Desliga o dispenser após 5 segundos
    if (racao_em_execucao) {
        if (absolute_time_diff_us(racao_acionamento_inicio, get_absolute_time()) > 5 * 1000000) {
            servo_set_angle(RACAO_PIN, 180);
            //gpio_put(RACAO_PIN, 0);
            racao_em_execucao = false;
            racao_acionada_manual = false;
            printf("[RTC] Dispenser de ração desligado.\n");
        }
    }

    // Reseta flag no início de um novo dia
    if (t->hour == 0 && t->min == 0) {
        racao1_acionada_hoje = false;
        racao2_acionada_hoje = false;
        printf("\n[RTC] Novo dia para ração!\n");
    }
}

// Verifica se é hora de acionar a limpeza e gerencia desligamento após 5s
void verificar_acionamento_limpeza(const datetime_t *t) {
    if (!rtc_running()) return;

    // Acionamento automático programado
    if (t->hour == hora_limpeza && t->min == minuto_limpeza && !limpeza_acionada_hoje && !limpeza_em_execucao) {
        printf("\n[RTC] HORA DA LIMPEZA! Acionando irrigadores.\n");
        gpio_put(LIMPEZA_PIN, 1);
        limpeza_acionamento_inicio = get_absolute_time();
        limpeza_em_execucao = true;
        limpeza_acionada_hoje = true;
    }

    // Desliga irrigadores após 5 segundos
    if (limpeza_em_execucao) {
        if (absolute_time_diff_us(limpeza_acionamento_inicio, get_absolute_time()) > 5 * 1000000) {
            gpio_put(LIMPEZA_PIN, 0);
            limpeza_em_execucao = false;
            limpeza_acionada_manual = false;
            printf("[RTC] Irrigadores desligados.\n");
        }
    }

    // Reseta flag no início de um novo dia
    if (t->hour == 0 && t->min == 0 && limpeza_acionada_hoje && !limpeza_em_execucao) {
        limpeza_acionada_hoje = false;
        printf("\n[RTC] Novo dia para limpeza!\n");
    }
}
