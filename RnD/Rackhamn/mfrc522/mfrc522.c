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

uint8_t rfid_transceive(uint8_t *send_data, uint8_t send_len, uint8_t *recv_data, uint8_t *recv_len) {
    uint8_t irq_en = 0x77;   // Enable RX, TX, ERR IRQs
    uint8_t wait_irq = 0x30; // Wait for RX IRQ or Idle IRQ
    uint32_t timeout = 10000; // Timeout counter

    // Enable interrupts and clear pending IRQs
    write_reg(0x02, irq_en | 0x80);  // Enable IRQ + global IRQ enable - CommIEnReg
    write_reg(0x04, 0x7F);           // Clear all IRQ flags - CommIrqReg

    // Flush FIFO
    write_reg(0x0A, 0x80); // Flush FIFO
    write_reg(0x01, 0x00); // Stop any active command, IDLE

    // Write to FIFO
    for (uint8_t i = 0; i < send_len; i++) {
        write_reg(0x09, send_data[i]); // FIFODataReg
    }

    // Adjust BitFramingReg (7-bit TX mode if needed)
    write_reg(0x0D, 0x00); // Default: 8-bit per byte

    // Start transmission (Transceive command)
    write_reg(0x01, 0x0C); // CommandReg, TRANSCEIVE
    set_bit_mask(0x0D, 0x80); // BitFramingReg Start transmission

    // Wait for IRQ
    uint8_t irq_reg = 0;
    while (timeout--) {
        irq_reg = read_reg(0x04); // CommIrqReg
        if (irq_reg & wait_irq) break;
    }
    if (timeout == 0) {
        printf("rfid_transceive: Timeout waiting for response\n");
        return MI_ERR;
    }

    // Check for errors
    uint8_t error = read_reg(0x06); // ErrorReg
    if (error & 0x1B) { // Check collision, parity, or buffer overflow errors
        printf("rfid_transceive: Error detected (0x%02X)\n", error);
        return MI_ERR;
    }

    // Ensure FIFO has data
    uint8_t attempts = 10;
    while (!(read_reg(0x0A)) && attempts--) {
        sleep_ms(1);
    }

    uint8_t fifo_level = read_reg(0x0A); // FIFOLevelReg
    if (fifo_level > *recv_len) {
        fifo_level = *recv_len; // Prevent buffer overflow
    }

    // Read FIFO
    for (uint8_t i = 0; i < fifo_level; i++) {
        recv_data[i] = read_reg(0x09); // FIFODataReg
    }

    *recv_len = fifo_level; // Update received length

    return MI_OK;
}

uint8_t card_command(uint8_t command, uint8_t * send_data, uint8_t send_len, uint8_t * back_data, uint8_t * back_len) {

        uint8_t status = MI_ERR;
        uint8_t irq_en = 0x77;
        uint8_t wait_irq = 0x30;

        switch(command) {
                // auth
                case 0x0E: {
                        irq_en = 0x12;
                        wait_irq = 0x10;
                } break;
                // transceive
                case 0x0C: {
                        irq_en = 0x77;
                        wait_irq = 0x30;
                } break;
        }

        *back_len = 0;

        write_reg(0x02, irq_en | 0x80);
        clear_bit_mask(0x04, 0x80);
        set_bit_mask(0x0A, 0x80);
        write_reg(0x01, 0x00);

        for(uint8_t i = 0; i < send_len; i++) {
                write_reg(0x09, send_data[i]);
        }

        write_reg(0x01, command);
        if(command == 0x0C) {
                set_bit_mask(0x0D, 0x80);
        }

	uint16_t timeout = 2000;
        uint8_t irq_reg = 0;
        do {
                irq_reg = read_reg(0x04);
                timeout--;
        } while(!(irq_reg & 0x30) && timeout > 0);
        if(timeout == 0) {
                // printf("card_command: read timeout!\n");
                return MI_ERR;
        }

        clear_bit_mask(0x0D, 0x80);

        if(read_reg(0x06) & 0x1B) {
                printf("card_command: error 0x1B\n");
                return MI_ERR;
        }

        if(command == 0x0C) {
                // FIFOLevelReg
                uint8_t n = read_reg(0x0A);
                if(n >= 16) n = 16;
                *back_len = n;

                // FIFODataReg
                for(uint8_t i = 0; i < n; i++) {
                        back_data[i] = read_reg(0x09);
                }
        }

        return MI_OK;
}

void rfid_calculate_crc(uint8_t * data, uint8_t len, uint8_t * crc) {
        write_reg(0x01, 0x00);
        write_reg(0x05, 0x04); // clear rcr irq
        write_reg(0x0A, 0x80);

        for(uint8_t i = 0; i < len; i++) {
                write_reg(0x09, data[i]);
        }

        // start crc calc
        write_reg(0x01, 0x03);

        for(uint16_t i = 5000; i > 0; i--) {
                if(read_reg(0x05) & 0x04) {
                        // crc irq bit is set
                        break;
                }
        }

        crc[0] = read_reg(0x22); // crc result reg low
        crc[1] = read_reg(0x21); // crc result reg highi
}

void rfid_halt(void) {
        uint8_t buf[4] = { 0x50, 0x00 }; // 0x50 = picc_halt

        rfid_calculate_crc(buf, 2, &buf[2]);

        uint8_t len;
        uint8_t status = card_command(0x0C, buf, 4, buf, &len);
        /*
        uint8_t response[1];
        uint8_t response_len = 1;
        uint8_t result = rfid_transceive(halt_cmd, 4, response, &response_len);
        if(result == MI_OK && response_len == 0) {
                return MI_OK;
        }

        return MI_ERR;
        */
}

uint8_t rfid_anticoll(uint8_t * data, uint8_t * len) {
        // printf("attempt anticoll\n");
        // BitFramingReg
        write_reg(0x0D, 0x00);
        // PICC_ANITCOLL, NVB
        // NVB = number of valid bits, with 0x93, 0x20 : 5-bytes (4-uid + 1 bcc checksum)
        // 0x93 : 4-byte uid, 0x95 : 7-byte uid, 0x97 : 10-byte uid
        uint8_t buf[2] = { 0x93, 0x20 };
        uint8_t buflen = 2;

        uint8_t back_data[16] = { 0 };
        uint8_t back_data_len = 0;

        uint8_t status = card_command(0x0C, buf, buflen, back_data, &back_data_len);
        // printf("cc : %i, len: %i\n", status, back_data_len);

#if defined(TEST)
        if(back_data_len != 0) {
                printf("anticoll: ");
                for(int i = 0; i < back_data_len; i++) {
                        printf("%02X ", back_data[i]);
                }
                printf("\n");
        }
#endif

        // we only handle 5 bytes (4-byte uid + checkbyte)
        if(status == MI_OK && back_data_len == 5) {
                memcpy(data, back_data, sizeof(back_data[0]) * back_data_len);
                *len = back_data_len;
                return MI_OK;
        }

        // printf("anticoll failed\n");
        return MI_ERR;
}

uint8_t rfid_request(uint8_t mode, uint8_t * tag_type) {
        // BitFramingReg
        write_reg(0x0D, 0x07);
        uint8_t status;
        uint8_t back_bits = 0;

        tag_type[0] = mode;
        // PCD_TRANSCEIVE or ControlReg
        status = card_command(0x0C, tag_type, 1, tag_type, &back_bits);

#if defined(TEST)
        printf("request status: %i, 0x%02X\n", status, back_bits);
        if(back_bits != 0) {
                printf("request: ");
                for(int i = 0; i < 2; i++) {
                        printf("%02X ", tag_type[i]);
                }
                printf("\n");
        }
#endif

        // discard error
        if((status != MI_OK) || (back_bits != 0x10)) {
                // return MI_ERR;
        }

        return status;
}

