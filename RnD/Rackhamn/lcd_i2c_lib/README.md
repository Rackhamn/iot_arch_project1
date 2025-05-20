# lcd_i2c_lib
A small I2C lcd library for interfacing with the 1602IIC from Tinsharp (TC1602B-01)

The interface is C-style pointer to struct passed to functions.  
To use this you just have to include the .h and .c in the same folder as your arduino sketch.  
8 custom characters can be built and written out.
They can always be overwritten.
```c
uint8_t cc_duck[8] = {0x0,0xc,0x1d,0xf,0xf,0x6,0x0};
uint8_t duck_id = 0;
lcd_make_custom_char(lcd, duck_id, cc_duck);

// once you've made your custom char the lcd screen needs to be reset with lcd_clear
// jumping back home beforehand is recommended
lcd_home(lcd);
lcd_clear(lcd);

lcd_custom_char(lcd, duck_id); // or regular char with lcd_char(lcd, 'a');
```

## Example: sketch.c (.ino)  
This example was made with the Arduino IDE and was originally an .ino file.  

Use the following snippet to be up and running.  
```c 
#include "lcd.h"

lcd_t _lcd, * lcd;

void setup() {
  lcd = &_lcd;
  // ptr, address, columns, rows, sda_pin, scl_pin
  // lcd_make(&_lcd, 0x27, 16, 2, 4, 5); <-- regular ptr_address also works
  lcd_make(lcd, 0x27, 16, 2, LCD_SDA, LCD_SCL);
}

void loop() {
  lcd_printf(lcd, "Hello %s", "*name*");
  lcd_set_cursor(lcd, 0, 1);
  lcd_printf(lcd, "Goodbye %s", "*name*");

  delay(3000);

  lcd_clear(lcd);
  lcd_home(lcd);
}
``` 
