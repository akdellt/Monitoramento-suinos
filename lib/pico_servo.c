#include "hardware/pwm.h"
#include "configura_geral.h"

#include "pico_servo.h"

uint32_t us_to_level(uint32_t us) {
    return (uint32_t)(us * 1.25f); // 1µs ≈ 1.25 unidades
}

// Função para inicializar o PWM
void servo_init(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(pin);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, PWM_CLK_DIV); // DIVISOR DE CLOCK
    pwm_config_set_wrap(&config, PWM_WRAP);      // WRAP
    pwm_init(slice_num, &config, true);
}

void servo_set_angle(uint pin, uint8_t angle) {
    if (angle > 180) angle = 180;

    // Interpolação linear para SG90
    uint32_t us = SERVO_MIN_US + ((SERVO_MAX_US - SERVO_MIN_US) * angle) / 180;

    pwm_set_gpio_level(pin, (uint32_t)(us * 1.25f));
}
