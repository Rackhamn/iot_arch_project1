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

void rfid_init(uint32_t spi_baud, spi_inst_t * spix, uint8_t sck_pin, uint8_t mosi_pin, uint8_t miso_pin, uint8_t cs_pin, uint8_t rst_pin) {
/*
        if(spix == 0)
                spi_init(spi0, spi_baud);
        else if(spix == 1)
                spi_init(spi1, spi_baud);
        else {
                printf("rfid_init :: ERROR - spix is not 0 or 1\n");
                return;
        }
*/
        _spi = spix;
        _sck = sck_pin;
        _mosi = mosi_pin;
        _miso = miso_pin;
        _cs = cs_pin;
        _rst = rst_pin;

        spi_init(_spi, spi_baud);

        gpio_set_function(_miso, GPIO_FUNC_SPI);
        gpio_set_function(_mosi, GPIO_FUNC_SPI);
        gpio_set_function(_sck, GPIO_FUNC_SPI);
        gpio_set_function(_cs, GPIO_FUNC_SIO);

        gpio_init(_cs);
        gpio_set_dir(_cs, GPIO_OUT);
        gpio_put(_cs, 1);

        gpio_init(_rst);
        gpio_set_dir(_rst, GPIO_OUT);
        gpio_put(_rst, 1);

        // write_reg(_rst, 1);
        reset();
        write_reg(0x2A, 0x8D);
        write_reg(0x2B, 0x3E);
        write_reg(0x2D, 30);
        write_reg(0x2C, 0);
        write_reg(0x15, 0x40);
        write_reg(0x11, 0x3D);
        antenna_on();
}


