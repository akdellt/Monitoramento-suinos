#include "pico/stdlib.h"
#include "configura_geral.h"
#include "pico_servo.h"

void acionadores_setup() {
    gpio_init(VENTILADOR_PIN);
    gpio_set_dir(VENTILADOR_PIN, GPIO_OUT);
    gpio_put(VENTILADOR_PIN, 0);

    gpio_init(LIMPEZA_PIN);
    gpio_set_dir(LIMPEZA_PIN, GPIO_OUT);
    gpio_put(LIMPEZA_PIN, 0);

    servo_init(RACAO_PIN);
    servo_set_angle(RACAO_PIN, 180);
}

void balancas_setup(){
    gpio_init(HX711_DATA);
    gpio_set_dir(HX711_DATA, GPIO_IN);
    gpio_pull_up(HX711_DATA);

    gpio_init(HX711_CLK);
    gpio_set_dir(HX711_CLK, GPIO_OUT);
    gpio_put(HX711_CLK, 0);
}