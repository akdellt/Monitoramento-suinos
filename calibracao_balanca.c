#include <stdio.h>
#include "pico/stdlib.h"

// Definições dos pinos do HX711
#define HX711_DOUT 9
#define HX711_SCK  8

// Fator de calibração — ajuste conforme sua célula de carga
#define SCALE_FACTOR 2280.0f

// Inicializa pinos do HX711
void hx711_initt(uint dout_pin, uint sck_pin) {
    gpio_init(dout_pin);
    gpio_set_dir(dout_pin, GPIO_IN);

    gpio_init(sck_pin);
    gpio_set_dir(sck_pin, GPIO_OUT);
    gpio_put(sck_pin, 0);
}

// Lê dados brutos do HX711 (24 bits)
int32_t hx711_read_raw(uint dout, uint sck) {
    while (gpio_get(dout));  // Espera até DOUT ficar LOW

    int32_t count = 0;
    for (int i = 0; i < 24; i++) {
        gpio_put(sck, 1);
        sleep_us(1);
        count = count << 1;
        gpio_put(sck, 0);
        sleep_us(1);
        if (gpio_get(dout)) count++;
    }

    // Pulso extra para ganho de 128
    gpio_put(sck, 1);
    sleep_us(1);
    gpio_put(sck, 0);
    sleep_us(1);

    // Conversão para signed
    if (count & 0x800000) count |= 0xFF000000;

    return count;
}

// Lê média de N leituras para reduzir ruído
int32_t hx711_read_average(uint dout, uint sck, int samples) {
    int64_t sum = 0;
    for (int i = 0; i < samples; i++) {
        sum += hx711_read_raw(dout, sck);
    }
    return (int32_t)(sum / samples);
}

int main() {
    stdio_init_all();
    hx711_initt(HX711_DOUT, HX711_SCK);

    sleep_ms(2000);  // Aguarda conexão serial

    printf("===== Calibração da Balança =====\n");
    printf("Remova qualquer peso da balança.\n");
    printf("Aguardando para medir o OFFSET (tara)...\n");
    sleep_ms(5000);

    int32_t offset = hx711_read_average(HX711_DOUT, HX711_SCK, 10);
    printf("OFFSET (tara) = %ld\n", offset);

    printf("\nColoque um peso conhecido na balança (ex: 2.00 kg)\n");
    sleep_ms(10000);

    int32_t leitura_com_peso = hx711_read_average(HX711_DOUT, HX711_SCK, 10);
    printf("Leitura com peso: %ld\n", leitura_com_peso);

    float peso_real = 0.395; // <-- ALTERE AQUI conforme o peso que colocou
    float scale = (float)(leitura_com_peso - offset) / peso_real;

    printf("\n===== Resultado =====\n");
    printf("OFFSET (tara)..............: %ld\n", offset);
    printf("LEITURA COM PESO...........: %ld\n", leitura_com_peso);
    printf("PESO REAL...................: %.3f kg\n", peso_real);
    printf("SCALE FACTOR (fator escala): %.3f\n", scale);
    printf("===============================\n");

    // Leitura contínua já calibrada
    while (1) {
        int32_t leitura = hx711_read_average(HX711_DOUT, HX711_SCK, 5);
        float peso = (float)(leitura - offset) / scale;
        printf("Peso atual: %.3f kg\n", peso);
        sleep_ms(1000);
    }

    return 0;
}