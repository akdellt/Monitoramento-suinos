#ifndef AHT10_H
#define AHT10_H

#include <stdbool.h>

void aht10_init();
void aht10_trigger_measurement();
bool aht10_read(float *temperature, float *humidity);

#endif