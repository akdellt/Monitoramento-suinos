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

#define AHT10_ADDR 0x38
#define I2C_PORT i2c0

#define I2C_PORT i2c0
#define PIN_SDA 0
#define PIN_SCL 1

//ALIMENTAÇÃO
// DEFINIÇÃO DOS VALORES DOS REGISTROS DE PWM PARA CÁLCULO DO DUTY CYCLE 
#define PWM_WRAP 24999  // 50HZ
#define PWM_CLK_DIV 100 // VALOR PARA DAR PERÍODO CERTO
#define SERVO_MIN_US 600     // Posição 0° no SG90
#define SERVO_MAX_US 2400    // Posição 180° no SG90

#define RACAO_PIN 2
extern int hora_racao;
extern int minuto_racao;

extern int hora_limpeza;
extern int minuto_limpeza;

// REFRIGERAÇÃO
extern float temp_maxima;
#define VENTILADOR_PIN 12
#define LIMPEZA_PIN 13

// DEFINIÇÃO DOS PINOS DO SENSOR DE PESO HX711
#define HX711_DATA 17  
#define HX711_CLK  18  
#define PESO_MIN_KG 5.0f
#define NUM_MAX_AMOSTRAS 50
#define SCALE_FACTOR 2100.0 / 5000.0 // CALIBRAR PARA 50/100KG
#define OFFSET 0.0f                     // VALOR INICIAL QUANDO BALANÇA ESTÁ NO ZERO

#define TEMPO_CONEXAO 2000
#define TEMPO_MENSAGEM 2000
#define TAM_FILA 16

#define TOPICO_TEMPERATURA      "suinosmonit/ambiente/temperatura"
#define TOPICO_UMIDADE          "suinosmonit/ambiente/umidade"
#define TOPICO_TEMPERATURA_LIMITE "suinosmonit/controle/temperatura_max"

#define TOPICO_RACAO            "suinosmonit/controle/racao"
#define TOPICO_RACAO_HORARIO_1  TOPICO_RACAO "/horario1"
#define TOPICO_RACAO_HORARIO_2  TOPICO_RACAO "/horario2"
#define TOPICO_RACAO_STATUS     TOPICO_RACAO "/status"

#define TOPICO_LIMPEZA          "suinosmonit/controle/limpeza"
#define TOPICO_LIMPEZA_HORARIO  TOPICO_LIMPEZA "/horarios"
#define TOPICO_LIMPEZA_STATUS   TOPICO_LIMPEZA "/status"

#define TOPICO_PORCO_BALANCA    "suinosmonit/porcos/peso"

#define MQTT_QOS 1

#define NTP_SERVER "pool.ntp.org"
#define NTP_MSG_LEN 48
#define NTP_PORT 123
#define NTP_DELTA 2208988800 // seconds between 1 Jan 1900 and 1 Jan 1970
#define NTP_TEST_TIME (30 * 1000)
#define NTP_RESEND_TIME (10 * 1000)

#endif