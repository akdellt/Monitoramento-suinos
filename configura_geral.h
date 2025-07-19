#ifndef CONFIGURA_GERAL_H
#define CONFIGURA_GERAL_H

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/cyw43_arch.h"
#include "pico/time.h"
#include <stdint.h>
#include <stdbool.h>
#include "pico/mutex.h"

// CONFIGURAÇÕES DO SENSOR AHT10
#define AHT10_ADDR 0x38 
#define I2C_PORT i2c1   
#define PIN_SDA 2       
#define PIN_SCL 3 

// CONFIGURAÇÕES DO SERVO MOTOR / DISPENSER DE RAÇÃO
#define PWM_WRAP 24999      // 50HZ
#define PWM_CLK_DIV 100 
#define SERVO_MIN_US 600    // POSIÇÃO 0° 
#define SERVO_MAX_US 2400   // POSIÇÃO 180°
#define RACAO_PIN 0

// CONFIGURAÇÃO DA BOMBA D'ÁGUA
#define LIMPEZA_PIN 4 

// VARÁVEIS DE CONTROLE DE TEMPO
extern int hora_racao;
extern int minuto_racao;

extern int hora_limpeza;
extern int minuto_limpeza;

// CONFIGUAÇÕES DO MOTOR DC / VENTILADOR
extern float temp_maxima;
#define VENTILADOR_PIN 28

// CONFIGURAÇÕES DA BALANÇA
#define HX711_DATA 9
#define HX711_CLK  8  
#define PESO_MIN_KG 0.200f          // ALTERÁVEL, EM APLICAÇÃO REAL INSERIR VALOR ALTO O SUFICIENTE PARA EVITAR LEITURAS ACIDENTAIS
#define NUM_MAX_AMOSTRAS 50
#define SCALE_FACTOR 180789.219f    // CALIBRAR PARA 10KG
#define OFFSET 6695.0f              // VALOR BRUTO INICIAL QUANDO BALANÇA ESTÁ SEM PESO


// TÓPICOS MQTT
#define TOPICO_TEMPERATURA          "suinosmonit/ambiente/temperatura"
#define TOPICO_UMIDADE              "suinosmonit/ambiente/umidade"
#define TOPICO_TEMPERATURA_LIMITE   "suinosmonit/controle/temperatura_max"

#define TOPICO_RACAO                "suinosmonit/controle/racao"
#define TOPICO_RACAO_HORARIO_1      TOPICO_RACAO "/horario1"
#define TOPICO_RACAO_HORARIO_2      TOPICO_RACAO "/horario2"
#define TOPICO_RACAO_STATUS         TOPICO_RACAO "/status"

#define TOPICO_LIMPEZA              "suinosmonit/controle/limpeza"
#define TOPICO_LIMPEZA_HORARIO      TOPICO_LIMPEZA "/horarios"
#define TOPICO_LIMPEZA_STATUS       TOPICO_LIMPEZA "/status"

#define TOPICO_PORCO_BALANCA        "suinosmonit/porcos/peso"

// QOS GERAL PARA TODOS OS TÓPICOS
#define MQTT_QOS 1


// CONFIGURAÇÕES DO NTP
#define NTP_SERVER "pool.ntp.org"
#define NTP_MSG_LEN 48
#define NTP_PORT 123
#define NTP_DELTA 2208988800        // SEGUNDOS ENTRE 1 JAN 1900 E 1 JAN 1970
#define NTP_TEST_TIME (30 * 1000)
#define NTP_RESEND_TIME (10 * 1000)

#endif