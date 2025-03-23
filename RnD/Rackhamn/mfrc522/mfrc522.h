#ifndef MFRC522_H
#define MFRC522_H

#define MI_OK		0
#define MI_NOTAGERR	1
#define MI_ERR		2

#define REQIDL		0x26
#define AUTH_A		0x60
#define AUTH_B		0x61

// TODO: add defines for all hex values in .c

// mfrc522 library state data
spi_inst_t * _spi;
uint8_t _sck, _mosi, _miso;
uint8_t _cs, _rst;

// TODO: convert to rfid_xxx or move wreg/rreg & bitmask to .c
void write_reg(uint8_t reg, uint8_t val);
uint8_t read_reg(uint8_t reg);

void set_bit_mask(uint8_t reg, uint8_t mask);
void clear_bit_mask(uint8_t reg, uint8_t mask);

void antenna_on(void);
void reset(void);

void rfid_init(uint32_t spi_baud, spi_inst_t * spix, uint8_t sck_pin, uint8_t mosi_pin, uint8_t miso_pin, uint8_t cs_pin, uint8_t rst_pin);

uint8_t rfid_transceive(uint8_t *send_data, uint8_t send_len, uint8_t *recv_data, uint8_t *recv_len);

uint8_t card_command(uint8_t command, uint8_t * send_data, uint8_t send_len, uint8_t * back_data, uint8_t * back_len);

void rfid_calculate_crc(uint8_t * data, uint8_t len, uint8_t * crc);

void rfid_halt(void);

uint8_t rfid_anticoll(uint8_t * data, uint8_t * len);

uint8_t rfid_request(uint8_t mode, uint8_t * tag_type);

/*
0x08    MIFARE Classic 1K/4K
0x09    MIFARE Mini (320 bytes)
0x18    MIFARE DESFire
0x20    NFC Forum Type 2
0x28    NFC Forum Type 4
*/
uint8_t rfid_get_sak(uint8_t * atqa, uint8_t * uid);

uint8_t rfid_auth(uint8_t picc_auth_mode, uint8_t sector, uint8_t * key, uint8_t * uid);

uint8_t rfid_write_block(uint8_t block, uint8_t * data);
uint8_t rfid_read_block(uint8_t block, uint8_t * data);

void rfid_dump_classic_1k(uint8_t * keya, uint8_t * keyb, uint8_t * uid);

#endif /* MFRC522_H */
