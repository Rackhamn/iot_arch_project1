#ifndef LCD_LIB_H
#define LCD_LIB_H

#if defined(__cplusplus)
extern "C" 
{
#endif

/* 
https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_i2c/include/hardware/i2c.h
https://github.com/cristiancristea00/Pico-I2C-LCD/blob/main/LCD_I2C.cpp
https://github.com/lawrencejones/raspi-gpio/blob/master/src/i2c/i2c_res.h
https://github.com/johnrickman/LiquidCrystal_I2C

1602IIC sold by freenove:
manufacturer: TINSHARP
model: TC1602B-01

operating voltage:      -0.3V to +7.0V
driver supply voltage:  -12V to +0.3V
input voltage range:    -0.3V to VDD +0.3V

E Cycle time:         min 1000 ns
E Pulse Width:        min 450 ns
E Rise/Fall time:     max 25 ns
Address Setup time:   min 60 ns
Address Hold time:    min 20 ns
Data output delay:    max 360 ns
Data hold time:       min 5.0 ns

Clear Display:      0000000001
Return Home:        000000001x

DDRAM: 80 bits
in 2-line display: 1st 0x00..0x39, 2nd 0x40..0x4F

TODO: add busy-wait handler with n-sized ring buffer commands
    + have read busy flag & address hadling
*/

// #include <Arduino.h>
#include <hardware/i2c.h>

// typedef byte uint8_t;

// 12 bytes :)
struct lcd_s {
  uint8_t addr;
  uint8_t cols;
  uint8_t rows;
  uint8_t backlight;

  // uint8_t cx, cy; // cursor

  // display
  uint8_t dpy_func;
  uint8_t dpy_ctrl;
  uint8_t dpy_mode;

  // i2c_inst * _i2c;
  i2c_inst_t * _i2c;
};
typedef struct lcd_s lcd_t;

#define LCD_I2C_BAUD_RATE   100000
#define LCD_PULSE_DELAY     600 // from the datasheet

// GPIO PINS
#define LCD_SDA             4 // PICO_DEFAULT_I2C_SDA_PIN
#define LCD_SCL             5 // PICO_DEFAULT_I2C_SCL_PIN

// Commands
#define LCD_CLEAR_DISPLAY   0x01
#define LCD_RETURN_HOME     0x02
#define LCD_ENTRY_MODE_SET  0x04
#define LCD_DISPLAY_CONTROL 0x08
#define LCD_CURSOR_SHIFT    0x10
#define LCD_FUNCTION_SET    0x20
#define LCD_SET_CGRAM_ADDR  0x40
#define LCD_SET_DDRAM_ADDR  0x80

// Flags for display entry mode set
#define LCD_ENTRY_RIGHT     0x00
#define LCD_ENTRY_LEFT      0x02

#define LCD_ENTRY_SHIFT_DECREMENT   0x00
#define LCD_ENTRY_SHIFT_INCREMENT   0x01

// Flags for display on/off control
#define LCD_BLINK_ON          0x01
#define LCD_BLINK_OFF         0x00
#define LCD_CURSOR_ON         0x02
#define LCD_CURSOR_OFF        0x00
#define LCD_DISPLAY_ON        0x04
#define LCD_DISPLAY_OFF       0x00

// Flags for cursor or display shift
#define LCD_MOVE_RIGHT        0x04
#define LCD_MOVE_LEFT         0x00
#define LCD_DISPLAY_MOVE      0x08
#define LCD_CURSOR_MOVE       0x00

// Flags for function set
#define LCD_MODE_8_BIT        0x10
#define LCD_MODE_4_BIT        0x00 // only one that works currently
#define LCD_LINE_2            0x08
#define LCD_LINE_1            0x00
#define LCD_DOTS_5x10         0x04
#define LCD_DOTS_5x8          0x00

// Flags for backlight control
#define LCD_BACKLIGHT         0x08
#define LCD_NO_BACKLIGHT      0x00

// Special flags
#define LCD_COMMAND           0x00
#define LCD_CHAR              0x01
#define LCD_REGISTER_SELECT   0x01
#define LCD_READ_WRITE        0x02
#define LCD_ENABLE            0x04

// ddgram
#define LCD_USER_SYMBOL_SIZE  8

// helper
#define LCD_TURN_BIT_ON(byte, flag)  (byte) |= (flag)
#define LCD_TURN_BIT_OFF(byte, flag) (byte) &= ~(flag)

#define lcd_write_command(lcdp, val) \
  lcd_write_byte((lcdp), (val), LCD_COMMAND)

// TODO: add status return
void lcd_make(lcd_t * lcd, uint8_t addr, uint8_t cols, uint8_t rows, uint8_t sda_pin, uint8_t scl_pin);
void lcd_init(lcd_t * lcd);

// these could prob be defines
void lcd_i2c_write_byte(lcd_t * lcd, uint8_t val);
void lcd_pulse_enable(lcd_t * lcd, uint8_t val);
void lcd_write_4bits(lcd_t * lcd, uint8_t val);
void lcd_write_byte(lcd_t * lcd, uint8_t val, uint8_t mode);
// void lcd_write_command(lcd_t * lcd, uint8_t val);

// these could be defines?
void lcd_display_on(lcd_t * lcd);
void lcd_display_off(lcd_t * lcd);
void lcd_backlight_on(lcd_t * lcd);
void lcd_backlight_off(lcd_t * lcd);

void lcd_cursor_blink_on(lcd_t * lcd);
void lcd_cursor_blink_off(lcd_t * lcd);
void lcd_cursor_on(lcd_t * lcd);
void lcd_cursor_off(lcd_t * lcd);
void lcd_clear(lcd_t * lcd);
void lcd_home(lcd_t * lcd);
void lcd_set_cursor(lcd_t * lcd, uint8_t col, uint8_t row);
void lcd_char(lcd_t * lcd, char c);
void lcd_string(lcd_t * lcd, const char * str);
size_t lcd_printf(lcd_t * lcd, const char * fmt, ...);

void lcd_scroll_left(lcd_t * lcd);
void lcd_scroll_right(lcd_t * lcd);

void lcd_register_select(lcd_t * lcd, uint8_t val);
void lcd_custom_char(lcd_t * lcd, uint8_t loc);
void lcd_make_custom_char(lcd_t * lcd, uint8_t loc, const uint8_t * buf);

#if defined(__cplusplus)
}
#endif

#endif /* LCD_LIB_H */
