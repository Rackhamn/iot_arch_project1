#include "lcd.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <Arduino.h> // delay, uint8_t ...

#include <hardware/gpio.h>
#include <hardware/i2c.h>
#include <pico/binary_info/code.h> // bi_decl

void lcd_make(lcd_t * lcd, uint8_t addr, uint8_t cols, uint8_t rows, uint8_t sda_pin, uint8_t scl_pin) {
  lcd->addr = addr;
  lcd->cols = cols;
  lcd->rows = rows;
  lcd->backlight = LCD_BACKLIGHT;

  lcd->_i2c = i2c0; // &i2c0_inst;
  i2c_init(lcd->_i2c, LCD_I2C_BAUD_RATE); // i2c_inst_t *, uint baudrate

  gpio_set_function(sda_pin, GPIO_FUNC_I2C);
  gpio_set_function(scl_pin, GPIO_FUNC_I2C);

  gpio_pull_up(sda_pin);
  gpio_pull_up(scl_pin);

  // make I2C pins avaiable to picotools
  // bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));
  lcd_init(lcd);
  lcd_clear(lcd);
  lcd_home(lcd);
}

void lcd_init(lcd_t * lcd) {
  lcd->dpy_mode = LCD_ENTRY_LEFT | LCD_ENTRY_SHIFT_DECREMENT;
  lcd->dpy_func = LCD_MODE_4_BIT | LCD_LINE_2 | LCD_DOTS_5x8;
  lcd->dpy_ctrl = LCD_DISPLAY_ON | LCD_CURSOR_ON | LCD_BLINK_OFF;

  // LiquidCrystal_I2C does this pattern
  // it also sleeps atleast 4.1ms (4500us)
  // as per the manufacturer datasheet
  lcd_write_command(lcd, 0x03);
  lcd_write_command(lcd, 0x03);
  lcd_write_command(lcd, 0x03);
  lcd_write_command(lcd, 0x02);

  lcd_write_command(lcd, LCD_ENTRY_MODE_SET | lcd->dpy_mode);
  lcd_write_command(lcd, LCD_FUNCTION_SET | lcd->dpy_func);

  lcd_display_on(lcd);
  lcd_clear(lcd);
  lcd_home(lcd);
}

void lcd_i2c_write_byte(lcd_t * lcd, uint8_t val) {
  static uint8_t data;

  // need backlight on always to actually see the shit
  data = val | lcd->backlight;
  i2c_write_blocking(lcd->_i2c, lcd->addr, &data, 1, false);
  // i2c_write_blocking_until
}

// for u4 data
void lcd_pulse_enable(lcd_t * lcd, uint8_t val) {
  sleep_us(LCD_PULSE_DELAY);
  lcd_i2c_write_byte(lcd, val | LCD_ENABLE);
  sleep_us(LCD_PULSE_DELAY);
  lcd_i2c_write_byte(lcd, val & ~LCD_ENABLE);
  sleep_us(LCD_PULSE_DELAY);
}

// for u4 data
void lcd_write_4bits(lcd_t * lcd, uint8_t val) {
  lcd_i2c_write_byte(lcd, val);
  lcd_pulse_enable(lcd, val);
}

void lcd_write_byte(lcd_t * lcd, uint8_t val, uint8_t mode) {
  #if 0
  static uint8_t high, low;

  // 0xf0 : 11110000
  high = val & 0xf0;
  low = (val << 4) & 0xf0;

  lcd_write_4bits(lcd, high | mode);
  lcd_write_4bits(lcd, low | mode);
  #else
  lcd_write_4bits(lcd, (val & 0xf0) | mode);
  lcd_write_4bits(lcd, ((val << 4) & 0xf0) | mode);
  #endif
}

/* @test
void lcd_write_command(lcd_t * lcd, uint8_t val) {
  lcd_write_byte(lcd, val, COMMAND);
}
*/

void lcd_display_on(lcd_t * lcd) {
  lcd->dpy_ctrl |= LCD_DISPLAY_ON;
  lcd_write_command(lcd, LCD_DISPLAY_CONTROL | lcd->dpy_ctrl);
}

void lcd_display_off(lcd_t * lcd) {
  lcd->dpy_ctrl &= ~LCD_DISPLAY_ON;
  lcd_write_command(lcd, LCD_DISPLAY_CONTROL | lcd->dpy_ctrl);
}

void lcd_backlight_on(lcd_t * lcd) {
  lcd->backlight = LCD_BACKLIGHT;
  lcd_i2c_write_byte(lcd, lcd->backlight);
}

void lcd_backlight_off(lcd_t * lcd) {
  lcd->backlight = LCD_NO_BACKLIGHT;
  lcd_i2c_write_byte(lcd, lcd->backlight);
}

void lcd_cursor_blink_on(lcd_t * lcd) {
  lcd->dpy_ctrl |= LCD_BLINK_ON;
  lcd_write_command(lcd, LCD_DISPLAY_CONTROL | lcd->dpy_ctrl);
}

void lcd_cursor_blink_off(lcd_t * lcd) {
  lcd->dpy_ctrl &= ~LCD_BLINK_ON;
  lcd_write_command(lcd, LCD_DISPLAY_CONTROL | lcd->dpy_ctrl);
}

void lcd_cursor_on(lcd_t * lcd) {
  // lcd->dpy_ctrl |= LCD_CURSOR_ON;
  LCD_TURN_BIT_ON(lcd->dpy_ctrl, LCD_CURSOR_ON);
  lcd_write_command(lcd, LCD_DISPLAY_CONTROL | lcd->dpy_ctrl);
}

void lcd_cursor_off(lcd_t * lcd) {
  // lcd->dpy_ctrl &= ~LCD_CURSOR_ON;
  LCD_TURN_BIT_OFF(lcd->dpy_ctrl, LCD_CURSOR_ON);
  lcd_write_command(lcd, LCD_DISPLAY_CONTROL | lcd->dpy_ctrl);
}

/* @combo
lcd->dpy_ctrl &= ~LCD_BLINK_ON;
lcd->dpy_ctrl |= LCD_CURSOR_ON;
lcd_write_command(lcd, LCD_DISPLAY_CONTROL | lcd->dpy_ctrl);
*/

void lcd_clear(lcd_t * lcd) {
  lcd_write_command(lcd, LCD_CLEAR_DISPLAY);
}

void lcd_home(lcd_t * lcd) {
  lcd_write_command(lcd, LCD_RETURN_HOME);
}

void lcd_autoscroll(lcd_t * lcd) {
  lcd->dpy_mode |= LCD_ENTRY_SHIFT_INCREMENT;
  lcd_write_command(lcd, LCD_ENTRY_MODE_SET | lcd->dpy_mode);
}

void lcd_set_cursor(lcd_t * lcd, uint8_t col, uint8_t row) {
  uint8_t row_offsets[4] = { 0x00, 0x40, (uint8_t)(0x00 + lcd->cols), (uint8_t)(0x40 + lcd->cols) };

  if(row < 0) row = 0;
  else if(row > lcd->rows) row = lcd->rows - 1;

  if(col < 0) col = 0;
  else if(col > lcd->cols) col = lcd->cols - 1;

  lcd_write_command(lcd, LCD_SET_DDRAM_ADDR | (row_offsets[row] + col));
}

void lcd_scroll_left(lcd_t * lcd) {
  lcd_write_command(lcd, LCD_CURSOR_SHIFT | LCD_DISPLAY_MOVE | LCD_MOVE_LEFT);
}

void lcd_scroll_right(lcd_t * lcd) {
  lcd_write_command(lcd, LCD_CURSOR_SHIFT | LCD_DISPLAY_MOVE | LCD_MOVE_RIGHT);
}

void lcd_register_select(lcd_t * lcd, uint8_t val) {
  lcd_write_byte(lcd, val, LCD_REGISTER_SELECT);
}

void lcd_custom_char(lcd_t * lcd, uint8_t loc) {
  lcd_write_byte(lcd, loc, LCD_REGISTER_SELECT);
}

void lcd_make_custom_char(lcd_t * lcd, uint8_t loc, const uint8_t * buf) {
  loc &= 0x7; // only 8 slots ... ret if oob?

  lcd_write_command(lcd, LCD_SET_CGRAM_ADDR | (loc << 3));
  for(size_t i = 0; i < LCD_USER_SYMBOL_SIZE; i++) {
    // lcd_register_select(lcd, (uint8_t)buf[i]);
    lcd_write_byte(lcd, (uint8_t)buf[i], LCD_REGISTER_SELECT);
  }

  delay(LCD_PULSE_DELAY);
}

void lcd_u8(lcd_t * lcd, uint8_t c) {
  lcd_write_byte(lcd, c, LCD_CHAR);
}

void lcd_char(lcd_t * lcd, char c) {
  lcd_write_byte(lcd, (uint8_t)c, LCD_CHAR);
}

void lcd_string(lcd_t * lcd, const char * str) {
  if(str == NULL) return;
  size_t len = strlen(str);
  for(int i = 0; i < len; i++) {
    lcd_write_byte(lcd, (uint8_t)str[i], LCD_CHAR);
  }
}

// max size 255 chars
size_t lcd_printf(lcd_t * lcd, const char * fmt, ...) {
  char buf[255] = { 0 };
  size_t w;
  va_list args;
  va_start(args, fmt);
  w = vsnprintf(buf, 255, fmt, args);
  va_end(args);
  lcd_string(lcd, buf);
  return w;
}
