#include "pico/stdlib.h"
#include "mqtt_client.h"
#include "pico/util/datetime.h"
#include "hardware/rtc.h"

#include "lib/mfrc522.h"
#include "balanca.h"
#include "configura_geral.h"

#include "FreeRTOS.h"
#include "task.h"

static const float scale_factor = SCALE_FACTOR;
static const float offset = OFFSET;

float somatorio = 0;
int num_amostras = 0;
bool coletando = false;
bool tag_lida = false;

MFRC522Ptr_t mfrc;

uint8_t porco1[] = {0xF6, 0xB9, 0x50, 0xC3};
uint8_t porco2[] = {0xE9, 0xF0, 0x78, 0x89};


void rfid_init(){
    mfrc = MFRC522_Init();
    PCD_Init(mfrc, spi0);
}


// --- LEITURA DA BALANÇA PARA VERIFICAR PESO
// VERIFICA SE O SENSOR ESTÁ PRONTO
bool hx711_is_ready() {
    return gpio_get(HX711_DATA) == 0;
}

// FUÇÃO DE LER O SENSOR DE CARGA
long hx711_read() {
    while (!hx711_is_ready()) sleep_us(10);

    int32_t  value = 0;
    for (int i = 0; i < 24; i++) {
        gpio_put(HX711_CLK, 1);
        sleep_us(1);
        value = (value << 1) | gpio_get(HX711_DATA);
        gpio_put(HX711_CLK, 0);
        sleep_us(1);
    }

    // PULSO EXTRA
    gpio_put(HX711_CLK, 1);
    sleep_us(1);
    gpio_put(HX711_CLK, 0);
    sleep_us(1);

    // AJUSTA SINAL NEGATIVO
    if (value & 0x800000) {
        value |= 0xFF000000; 
    }

    return value;
}

// FUNÇÃO PRA CONVERTER VALOR DO SENSOR PARA PESO
float convert_to_weight(long raw_value, float scale_factor, float offset) {
    return (raw_value - offset) / scale_factor;
}

// LÊ MÉDIA PARA EVITAR RUÍDOS
int32_t hx711_read_average(int samples) {
    int64_t sum = 0;
    for (int i = 0; i < samples; i++) {
        sum += hx711_read();
    }
    return (int32_t)(sum / samples);
}

// FUNÇÃO DE COLETA
void coleta_peso() {
    long raw_value = hx711_read_average(10);                 // ARMAZENA VALOR BRUTO DO SENSOR DE CARGA
    float peso = convert_to_weight(raw_value, scale_factor, offset); // TRANSFORMA BRUTO EM PESO
    if (peso < 0.1) peso = 0.0;
    //printf("%.3f \n", peso);

    if (peso >= PESO_MIN_KG) {
        if (!coletando) {
            coletando = true;
            somatorio = 0;
            num_amostras = 0;
            printf("Porco detectado na balança \n");
        }

        if (num_amostras < NUM_MAX_AMOSTRAS) {
            somatorio += peso;
            num_amostras++;
        }
    } else if (coletando && peso < PESO_MIN_KG) {
        coletando = false;
        tag_lida = false;

        if (num_amostras > 0) {
            float media = somatorio / num_amostras;

            somatorio = 0;
            num_amostras = 0;

            char payload[64];

            // DATA ATUAL 
            datetime_t agora;
            rtc_get_datetime(&agora);
            char data[32];
            snprintf(data, sizeof(data), "%02d/%02d/%04d", agora.day, agora.month, agora.year);
            char animal[10];

            printf("Leia tag do animal \n");
            absolute_time_t inicio = get_absolute_time();
            while (absolute_time_diff_us(inicio, get_absolute_time()) < 5 * 1000000) {
                if (PICC_IsNewCardPresent(mfrc)) {
                    if (PICC_ReadCardSerial(mfrc)) {
                        tag_lida = true;
                    }
                    break;
                }
            }

            if (tag_lida) {
                if (memcmp(mfrc->uid.uidByte, porco1, 4) == 0) {
                    snprintf(animal, sizeof(animal), "porco 1");
                } else if (memcmp(mfrc->uid.uidByte, porco2, 4) == 0) {
                    snprintf(animal, sizeof(animal), "porco 2");
                } 
            } else {
                snprintf(animal, sizeof(animal), "desconhecido");
                printf("Nenhuma tag reconhecida \n");
            }

            snprintf(payload, sizeof(payload), "{\"animal\": \"%s\", \"peso\": %.2f, \"data\": \"%s\"}", animal, media, data);

            if (mqtt_is_connected()) {
                mqtt_do_publish(mqtt_get_client(), TOPICO_PORCO_BALANCA, payload, NULL);
                printf("Peso do %s enviado: %.2f kg em %s\n", animal, media, data);
            }
        }
    }
}