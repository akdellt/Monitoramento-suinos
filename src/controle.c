#include "controle.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/rtc.h"
#include "pico/util/datetime.h"

#include "inc/pico_servo.h"
#include "configura_geral.h"

// --- VARIÁVEIS DE CONTROLE
// HORÁRIO RAÇÃO -- 6H E 18H
int hora_racao_1 = 6, hora_racao_2 = 18;
int minuto_racao_1 = 0, minuto_racao_2 = 0;

// HORÁRIO LIMPEZA -- 12H
int hora_limpeza = 12;
int minuto_limpeza = 0;

// CONTROLE DE ACIONAMENTO AUTOMÁTICO
static bool racao1_acionada_hoje, racao2_acionada_hoje = false;
static bool limpeza_acionada_hoje = false;

// CONTROLE DE ACIONAMENTO MANUAL
static bool racao_acionada_manual = false;
static bool limpeza_acionada_manual = false;

// VARIÁVEIS DE  CONTROLE DE TEMPO -- RAÇÃO
static bool racao_em_execucao = false;
static absolute_time_t racao_acionamento_inicio;

// VARIÁVEIS DE  CONTROLE DE TEMPO -- LIMPEZA
static bool limpeza_em_execucao = false;
static absolute_time_t limpeza_acionamento_inicio;

// VARIÁVEIS DE CONTROLE DO VENTILADOR
float temp_maxima = 30.f;
static bool ventilador_ligado = false;


// --- FUNÇÕES DE CONTROLE

// ACIONAMENTO DO VENTILADOR
void controle_ventilador(float temperatura) {
    //SE TEMPERATURA FOR MAIOR OU IGUAL A TEMPERATURA MÁXIMA E VENTILADOR NÃO ESTIVER LIGADO
    if (temperatura >= temp_maxima && !ventilador_ligado) {
        gpio_put(VENTILADOR_PIN, 1);
        ventilador_ligado = true;
        printf("Temperatura alta (%.1f°C). Ventilador LIGADO \n", temperatura);
    } 
    // SE TEMPERATURA ESTIVER 2 GRAUS ABAIXO DA TEMPERATURA MÁXIMA E VENTILADOR ESTIVER LIGADO
    else if (temperatura <= (temp_maxima - 2) && ventilador_ligado) {
        gpio_put(VENTILADOR_PIN, 0);
        ventilador_ligado = false;
        printf("Temperatura normalizada (%.1f°C). Ventilador DESLIGADO \n", temperatura);
    }
}

// CONTROLE HORÁRIO RAÇÃO
void atualizar_horario_racao(const char *momento, int hora, int minuto) {
    // SE MOMENTO FOR RAÇÃO 1, ATUALIZA HORÁRIO
    if (strcmp(momento, "racao_1") == 0) {
        hora_racao_1 = hora;
        minuto_racao_1 = minuto;
    }
    // SE MOMENTO FOR RAÇÃO 2, ATUALIZA HORÁRIO
    else if (strcmp(momento, "racao_2") == 0) {
        hora_racao_2 = hora;
        minuto_racao_2 = minuto;
    }
    
    printf("Novo horário da ração: %02d:%02d \n", hora, minuto);
}

// CONTROLE HORÁRIO LIMPEZA
void atualizar_horario_limpeza(int hora, int minuto) {
    hora_limpeza = hora;
    minuto_limpeza = minuto;
    printf("Novo horário da limpeza: %02d:%02d \n", hora, minuto);
}

// CONTROLE MANUAL DE RAÇÃO
void acionar_racao_manual() {
    // SE DISPENSER NÃO ESTIVER ACIONADO, LIBERA RAÇÃO 
    if (!racao_em_execucao) {
        printf("Acionando dispenser de ração! \n");
        servo_set_angle(RACAO_PIN, 90);
        racao_acionamento_inicio = get_absolute_time();     // ARMAZENA TEMPO DE ACIONAMENTO
        racao_em_execucao = true;
        racao_acionada_manual = true;
    } else {
        printf("Ração já em execução \n");
    }
}

//CONTROLE MANUAL DE LIMPEZA
void acionar_limpeza_manual() {
    // SE LIMPEZA NÃO ESTIVER ATIVADO, ACIONA SISTEMA
    if (!limpeza_em_execucao) {
        printf("Acionando irrigadores de limpeza! \n");
        gpio_put(LIMPEZA_PIN, 1);
        limpeza_acionamento_inicio = get_absolute_time();   // ARMAZENA TEMPO DE ACIONAMENTO
        limpeza_em_execucao = true;
        limpeza_acionada_manual = true;
    } else {
        printf("Limpeza já em execução \n");
    }
}

// VERIFICA SE É HORA DE ACIONAR RAÇÃO E GERENCIA DESLIGAMENTO APÓS 5S
void verificar_acionamento_racao(const datetime_t *t) {
    // VERIFICA SE RTC ESTÁ FUNCIONANDO
    if (!rtc_running()) return;

    // ACIONAMENTO AUTOMÁTICO PROGRAMADO - HORÁRIO 1
    if (t->hour == hora_racao_1 && t->min == minuto_racao_1 && !racao1_acionada_hoje && !racao_em_execucao) {
        printf("HORA DA PRIMEIRA RAÇÃO! Acionando dispenser \n");
        servo_set_angle(RACAO_PIN, 90);                     // ABRE DISPENSER
        racao_acionamento_inicio = get_absolute_time();     // ARMAZENA TEMPO DE ACIONAMENTO
        racao_em_execucao = true;
        racao1_acionada_hoje = true;
    }

    // ACIONAMENTO AUTOMÁTICO PROGRAMADO - HORÁRIO 2
    if (t->hour == hora_racao_2 && t->min == minuto_racao_2 && !racao2_acionada_hoje && !racao_em_execucao) {
        printf("\n[RTC] HORA DA SEGUNDA RAÇÃO! Acionando dispenser \n");
        servo_set_angle(RACAO_PIN, 90);                     // ABRE DISPENSER
        racao_acionamento_inicio = get_absolute_time();     // ARMAZENA TEMPO DE ACIONAMENTO
        racao_em_execucao = true;
        racao2_acionada_hoje = true;
    }

    // DESLIGA DISPENSER APÓS 5 SEGUNDOS
    if (racao_em_execucao) {
        if (absolute_time_diff_us(racao_acionamento_inicio, get_absolute_time()) > 5 * 1000000) {
            servo_set_angle(RACAO_PIN, 180);                // FECHA DISPENSER
            racao_em_execucao = false;
            racao_acionada_manual = false;
            printf("Dispenser de ração desligado \n");
        }
    }

    // RESETA FLAGS DEPOIS DA MEIA NOITE
    if (t->hour == 0 && t->min == 0) {
        racao1_acionada_hoje = false;
        racao2_acionada_hoje = false;
    }
}

// VERIFICA SE É HORA DE ACIONAR LIMPEZA E GERENCIA DESLIGAMENTO APÓS 5S
void verificar_acionamento_limpeza(const datetime_t *t) {
    // VERIFICA SE RTC ESTÁ FUNCIONANDO
    if (!rtc_running()) return;

    // ACIONAMENTO AUTOMÁTICO PROGRAMADO
    if (t->hour == hora_limpeza && t->min == minuto_limpeza && !limpeza_acionada_hoje && !limpeza_em_execucao) {
        printf("HORA DA LIMPEZA! Acionando irrigadores \n");
        gpio_put(LIMPEZA_PIN, 1);
        limpeza_acionamento_inicio = get_absolute_time();
        limpeza_em_execucao = true;
        limpeza_acionada_hoje = true;
    }

    // DESLIGA SISTEMA APÓS 5 SEGUNDOS
    if (limpeza_em_execucao) {
        if (absolute_time_diff_us(limpeza_acionamento_inicio, get_absolute_time()) > 5 * 1000000) {
            gpio_put(LIMPEZA_PIN, 0);
            limpeza_em_execucao = false;
            limpeza_acionada_manual = false;
            printf("Irrigadores desligados \n");
        }
    }

    // RESETA FLAGS DEPOIS DA MEIA NOITE
    if (t->hour == 0 && t->min == 0 && limpeza_acionada_hoje && !limpeza_em_execucao) {
        limpeza_acionada_hoje = false;
    }
}

