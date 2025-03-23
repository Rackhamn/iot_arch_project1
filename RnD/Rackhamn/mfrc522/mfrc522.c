#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <stdio.h>
#include <string.h>

#include "mfrc522.h"

void write_reg(uint8_t reg, uint8_t val) {
        uint8_t data[2] = { (reg << 1) & 0x7E, val };                                              gpio_put(_cs, 0);
        spi_write_blocking(_spi, data, 2);
        gpio_put(_cs, 1);
}

uint8_t read_reg(uint8_t reg) {
        uint8_t addr = ((reg << 1) & 0x7E) | 0x80;
        uint8_t val = 0;

        gpio_put(_cs, 0);
        spi_write_blocking(_spi, &addr, 1);                                                        spi_read_blocking(_spi, 0x00, &val, 1);
        gpio_put(_cs, 1);

        return val;
}

void set_bit_mask(uint8_t reg, uint8_t mask) {
        write_reg(reg, read_reg(reg) | mask);
}

void clear_bit_mask(uint8_t reg, uint8_t mask) {
        write_reg(reg, read_reg(reg) & (~mask));
}

void antenna_on(void) {
        if(~(read_reg(0x14) & 0x03)) {
                set_bit_mask(0x14, 0x03);
        }
}

void reset(void) {
        write_reg(0x01, 0x0F);
        sleep_ms(50); // ..
}


