# ssd1306-fixed-font

Display 8x8 fixed font to ssd1306 OLED.   

---

# i2c Connection

Enable this line if your display connected by i2c   
__#define I2C_CONNECTION__   

|OLED(i2c)||ESP8266|
|:-:|:-:|:-:|
|GND|--|N/C|
|VDD|--|3.3V|
|SCK|--|GPIO5|
|SDA|--|GPIO4|

---

# SPI Connection

Remove this line if your display connected by SPI   
__#define I2C_CONNECTION__   

|OLED(SPI)||ESP8266|
|:-:|:-:|:-:|
|GND|--|GND|
|VCC|--|3.3V|
|DO|--|GPIO14|
|DI|--|GPIO13|
|RES|--|GPIO0(*)|
|DC|--|GPIO4(*)|
|CS|--|GPIO5(*)|

\*You can change any GPIO.   

---

# Font File

The original version written by Marcel Sondaar is availble at:
https://github.com/dhepper/font8x8
