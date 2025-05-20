#!/bin/zsh

BASE_PATH="RnD/Gabbemannen00/pico_w"

git add $BASE_PATH/IP_adress_check.py && git commit -m "Check and print 
Pico W IP address for network connectivity"
git add $BASE_PATH/LCD_ON_OFF.py && git commit -m "Toggle LCD display 
on/off for power saving"
git add $BASE_PATH/LCD_api.py && git commit -m "Provide API functions to 
interact with the LCD display"
git add $BASE_PATH/LCD_check.py && git commit -m "Perform diagnostics and 
status checks on LCD display"
git add $BASE_PATH/LCD_talking.py && git commit -m "Handle higher-level 
message display on the LCD"
git add $BASE_PATH/MFRC522.py && git commit -m "Driver for MFRC522 RFID 
reader module"
git add $BASE_PATH/RFID_blueblink.py && git commit -m "Blink blue LED 
based on RFID read events"
git add $BASE_PATH/RFID_buzzer.py && git commit -m "Control buzzer based 
on RFID tag detections"
git add $BASE_PATH/RFID_buzzer_leds_lcd.py && git commit -m "Integrate 
buzzer, LEDs and LCD for RFID events"
git add $BASE_PATH/RFID_check.py && git commit -m "Validate and check RFID 
tag data"
git add $BASE_PATH/SDcard.py && git commit -m "Utilities for SD card 
read/write operations"
git add $BASE_PATH/TCP_server.py && git commit -m "Implement TCP server 
for communication with Pico W"
git add $BASE_PATH/WIFI_manager.py && git commit -m "Manage WiFi 
connection and reconnection for Pico W"
git add $BASE_PATH/i2c_lcd.py && git commit -m "I2C interface driver for 
LCD module"
git add $BASE_PATH/main.py && git commit -m "Main application logic for 
Pico W startup and control"
git add $BASE_PATH/lib/ssd1306.py && git commit -m "Driver for SSD1306 
OLED display module"

