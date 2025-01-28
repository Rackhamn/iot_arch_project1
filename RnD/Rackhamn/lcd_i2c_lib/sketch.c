// sketch.ino
#include "lcd.h"

lcd_t _lcd, * lcd;

// 5x8 custom character examples 
// chars from: https://github.com/johnrickman/LiquidCrystal_I2C/blob/master/examples/CustomChars/CustomChars.pde
uint8_t cc_bell[8]  = {0x4,0xe,0xe,0xe,0x1f,0x0,0x4};
uint8_t cc_note[8]  = {0x2,0x3,0x2,0xe,0x1e,0xc,0x0};
uint8_t cc_clock[8] = {0x0,0xe,0x15,0x17,0x11,0xe,0x0};
uint8_t cc_heart[8] = {0x0,0xa,0x1f,0x1f,0xe,0x4,0x0};
uint8_t cc_duck[8]  = {0x0,0xc,0x1d,0xf,0xf,0x6,0x0};
uint8_t cc_check[8] = {0x0,0x1,0x3,0x16,0x1c,0x8,0x0};
uint8_t cc_cross[8] = {0x0,0x1b,0xe,0x4,0xe,0x1b,0x0};
uint8_t cc_retarrow[8] = {0x1,0x1,0x5,0x9,0x1f,0x8,0x4};

// lowercase ä
uint8_t cc_a2[8] = {
  0b00001010,
  0x0,
  0b00001110,
  0b00010001,
  0b00011111,
  0b00010001,
  0b00010001,
  0x0
};

// 115200
#define SERIAL_BAUD   9600
void setup_usb_serial() {
  Serial.begin(SERIAL_BAUD);
  while (!Serial && millis() < 10000UL);
  Serial.println("Serial started"); 
}

void setup() {
  setup_usb_serial();

  lcd = &_lcd;
  
  // ptr, address, width, height
  make_lcd(lcd, 0x27, 16, 2, LCD_SDA, LCD_SCL);

  lcd_make_custom_char(lcd, 0, cc_duck);
  lcd_clear(lcd);
  lcd_home(lcd);
  
  lcd_cursor_off(lcd);

  lcd_string(lcd, "Hello! ");
  lcd_custom_char(lcd, 0);
  lcd_set_cursor(lcd, 0, 1);
  lcd_printf(lcd, "lcd size: %li", sizeof(struct lcd_s));
}

void loop() {
  #if 0
  // TODO: handle input as if its a TTY
  // keep copy state of messages + cursor position
  // handle protocol message and not just straight data
  // note: can we turn this into async? (non-blocking)

  while(Serial.available() > 0) {
    int val = Serial.read();

    if(val == '\\') {
      if(Serial.available() > 0) {
        val = Serial.read();

        if(val == 'r') {
          lcd_home(lcd);
        }

        if(val == 'n') {
          lcd_set_cursor(lcd, 0, 1);
        }
      }
      continue;
    }

    if(val == '.') {
      lcd_clear(lcd);
      continue;
    }

    if(val == ',') {
      lcd_home(lcd);
      continue;
    }

    if(val > 0) {
      lcd_u8(lcd, (uint8_t)val);
    }
  }
  #endif
}