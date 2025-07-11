#ifndef PICO_SERVO_H
#define PICO_SERVO_H

uint32_t us_to_level(uint32_t us);

void servo_init(uint pin);

void servo_set_angle(uint pin, uint8_t angle);

#endif