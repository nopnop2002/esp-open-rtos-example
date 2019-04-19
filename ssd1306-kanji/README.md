# ssd1306-kanji

Display Japanese Font to ssd1306 OLED.   

__This example require 4MByte FLASH size.__   
__Before build program, Make erase all Flash.__   

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

![ssd1306-1](https://user-images.githubusercontent.com/6020549/56445688-7cd30280-6339-11e9-8013-ef0122278d70.JPG)

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

![ssd1306-2](https://user-images.githubusercontent.com/6020549/56445691-7fcdf300-6339-11e9-9def-d86b776e68c1.JPG)

---

Left i2c   
Right SPI   

![ssd1306-3](https://user-images.githubusercontent.com/6020549/56445693-82c8e380-6339-11e9-9bc8-a6cc12cd51c1.JPG)

