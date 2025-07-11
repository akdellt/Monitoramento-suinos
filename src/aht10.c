#include "aht10.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include "configura_geral.h"


void aht10_init() {
    i2c_init(I2C_PORT, 100 * 1000);
    gpio_set_function(PIN_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_SDA);
    gpio_pull_up(PIN_SCL);

    uint8_t cmd[] = {0xBE, 0x08, 0x00};
    i2c_write_blocking(I2C_PORT, AHT10_ADDR, cmd, 3, false);
    sleep_ms(20);
}

void aht10_trigger_measurement() {
    uint8_t cmd[] = {0xAC, 0x33, 0x00};
    i2c_write_blocking(I2C_PORT, AHT10_ADDR, cmd, 3, false);
    sleep_ms(80);
}

bool aht10_read(float *temperature, float *humidity) {
    uint8_t raw_data[6];
    i2c_read_blocking(I2C_PORT, AHT10_ADDR, raw_data, 6, false);

    if ((raw_data[0] & 0x80) == 0x80) {
        return false;
    }

    uint32_t raw_humidity = ((raw_data[1] << 12) | (raw_data[2] << 4) | (raw_data[3] >> 4));
    uint32_t raw_temperature = (((raw_data[3] & 0x0F) << 16) | (raw_data[4] << 8) | raw_data[5]);

    *humidity = (raw_humidity / 1048576.0f) * 100.0f;
    *temperature = (raw_temperature / 1048576.0f) * 200.0f - 50.0f;

    return true;
}
