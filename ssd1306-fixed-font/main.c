#include <espressif/esp_common.h>
#include <esp/uart.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <timers.h>
#include <string.h>
#include <ssd1306/ssd1306.h>

/* Remove this line if your display connected by SPI */
#define I2C_CONNECTION

#ifdef I2C_CONNECTION
    #include <i2c/i2c.h>
#endif

#include "font8x8_basic.h"
#include "font8x8_greek.h"
#include "font8x8_ext_latin.h"

/* Change this according to you schematics and display size */
#define DISPLAY_WIDTH  128
#define DISPLAY_HEIGHT 64

#ifdef I2C_CONNECTION
    #define PROTOCOL SSD1306_PROTO_I2C
    #define ADDR     SSD1306_I2C_ADDR_0
    #define I2C_BUS  0
    #define SCL_PIN  5
    #define SDA_PIN  4
#else
    #define PROTOCOL SSD1306_PROTO_SPI4
    #define CS_PIN   5
    #define DC_PIN   4
#endif

/* Declare device descriptor */
static const ssd1306_t dev = {
    .protocol = PROTOCOL,
#ifdef I2C_CONNECTION
    .i2c_dev.bus      = I2C_BUS,
    .i2c_dev.addr     = ADDR,
#else
    .cs_pin   = CS_PIN,
    .dc_pin   = DC_PIN,
#endif
    .width    = DISPLAY_WIDTH,
    .height   = DISPLAY_HEIGHT
};

/* Local frame buffer */
static uint8_t buffer[DISPLAY_WIDTH * DISPLAY_HEIGHT / 8];
static uint8_t invert_buffer[DISPLAY_WIDTH * DISPLAY_HEIGHT / 8];


#define SECOND (1000 / portTICK_PERIOD_MS)

// 8ビットデータを反転 0x12(00010010)->0x48(01001000)
uint8_t RotateByte(uint8_t ch1) {
  uint8_t ch2 = 0;
  int j;
  for (j=0;j<8;j++) {
    ch2 = (ch2 << 1) + (ch1 & 0x01);
    ch1 = ch1 >> 1;
  }
  return ch2;
}

// Convert font to bitmap
void MakeBitmap(char *fonts, char *line, uint8_t w, uint8_t h) {
  int x,y;
  for(y=0; y<(h/8); y++){
    for(x=0; x<w; x++){
      line[y*32+x] = 0;
    }
  }

  int mask = 1; //7;
  int fontp;
  fontp = 0;
  for(y=0; y<h; y++){
    for(x=0; x<w; x++){
      uint8_t d = fonts[fontp+x/8];
      uint8_t linep = (y/8)*32+x;
      //if (d & (0x80 >> (x % 8))) line[linep] = line[linep] + (1 << mask);
      if (d & (0x80 >> ((7-x) % 8))) line[linep] = line[linep] + (1 << mask);
    }
    mask++; //mask--;
    if (mask > 0x7) mask = 1;
    fontp += (w + 7)/8;
  }
}

// 上下＋左右の反転
void ssd1306_invert_frame(int width, int height, int xofs, int yofs, uint8_t * src, uint8_t * dst) {
    int page = (height - yofs) / 8;
    int last = width * page - 1;
//printf("page=%d last=%d xofs=%d yofs=%d\n",page,last,xofs,yofs);
    for(int y=0;y<page;y++) {
      int dpos = last - (y*width);
      int spos = y * width + xofs;
//printf("page=%d last=%d dpos=%d spos=%d\n",page,last,dpos,spos);
      for(int x=xofs;x<width;x++) {
        dst[dpos--] = RotateByte(src[spos++]);
      }
    }
    return;
}

static void ssd1306_invert_text(uint8_t *buf, size_t blen)
{
    uint8_t wk;
    for(int i=0; i<blen; i++){
        wk = buf[i];
        buf[i] = ~wk;
    }
}


static int ssd1306_print_text(char font[][8], uint8_t * buf, int page, int seg, char * str, int strlen, bool flag) {
    int _seg = seg;
    int pos = page * 128 + seg;
    char bitmap[8];
    for(int i=0;i<strlen;i++) {
        int ch = str[i];
    	//memcpy(&buf[pos], font8x8_basic[ch],8);
        MakeBitmap(font[ch], bitmap, 8, 8);
    	memcpy(&buf[pos], bitmap, 8);
        if (flag) ssd1306_invert_text(&buf[pos], 8);
        pos = pos + 8;
        _seg = _seg + 8;
    }
    return _seg;
}

static void ssd1306_print_text_left(char font[][8], uint8_t * buf, int page, char * str, int strlen, bool flag) {
    ssd1306_print_text(font, buf, page, 0, str, strlen, flag);
}

static void ssd1306_print_text_right(char font[][8], uint8_t * buf, int page, char * str, int strlen, bool flag) {
    int pos = (16 - strlen) * 8;
    ssd1306_print_text(font, buf, page, pos, str, strlen, flag);
}

static void ssd1306_print_text_center(char font[][8], uint8_t * buf, int page, char * str, int strlen, bool flag) {
    int pos = (16 - strlen) * 4;
    ssd1306_print_text(font, buf, page, pos, str, strlen, flag);
}

static void ssd1306_clear_page(uint8_t * buf, int page, bool flag) {
    char str[16];
    memset(str, 0x20, 16);
    ssd1306_print_text(font8x8_basic, buf, page, 0, str, 16, flag);
}

static void ssd1306_task(void *pvParameters)
{
    printf("%s: Started user interface task\n", __FUNCTION__);
    vTaskDelay(1000/portTICK_PERIOD_MS);
    char text[20];

#if 0
    ssd1306_fill_rectangle(&dev, buffer, 0, 0, DISPLAY_WIDTH,
			 DISPLAY_HEIGHT/2, OLED_COLOR_WHITE);
#endif

    ssd1306_print_text_center(font8x8_basic, buffer, 0, "0123456789", 10, false);
    ssd1306_print_text_left(font8x8_basic, buffer, 1, "0123456789", 10, false);
    ssd1306_print_text_right(font8x8_basic, buffer, 2, "0123456789", 10, false);

    int pos = 0;
    pos = ssd1306_print_text(font8x8_basic, buffer, 4, pos, "1", 1, false);
    text[0] = 3; //pound sterling
    pos = ssd1306_print_text(font8x8_ext_latin, buffer, 4, pos, text, 1, false);
    pos = ssd1306_print_text(font8x8_basic, buffer, 4, pos, "=", 1, false);
    text[0] = 5; // yen
    pos = ssd1306_print_text(font8x8_ext_latin, buffer, 4, pos, text, 1, false);
    pos = ssd1306_print_text(font8x8_basic, buffer, 4, pos, "139", 3, false);


    pos = 64;
    pos = ssd1306_print_text(font8x8_basic, buffer, 4, pos, "0", 1, false);
    text[0] = 16; // degree
    pos = ssd1306_print_text(font8x8_ext_latin, buffer, 4, pos, text, 1, false);
    pos = ssd1306_print_text(font8x8_basic, buffer, 4, pos, "C=32", 4, false);
    text[0] = 16; // degree
    pos = ssd1306_print_text(font8x8_ext_latin, buffer, 4, pos, text, 1, false);
    pos = ssd1306_print_text(font8x8_basic, buffer, 4, pos, "F", 1, false);

#if 0
    for(int i=0;i<10;i++) text[i] = i;
    ssd1306_print_text_center(font8x8_greek, buffer, 5, text, 10, false);
#endif

    while(1) {
        TickType_t nowTick = xTaskGetTickCount();
	sprintf(text, "Tick:%d", nowTick);
	ssd1306_clear_page(buffer, 6, false);
	ssd1306_print_text_center(font8x8_basic, buffer, 6, text, strlen(text), false);
	ssd1306_clear_page(buffer, 7, true);
	ssd1306_print_text_center(font8x8_basic, buffer, 7, text, strlen(text), true);
        ssd1306_invert_frame(DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, 0, buffer, invert_buffer);
        if (ssd1306_load_frame_buffer(&dev, invert_buffer)) {
    	    printf("%s: error while loading framebuffer into SSD1306\n", __func__)
;
            while(1) {
                vTaskDelay(2 * SECOND);
                printf("%s: error loop\n", __FUNCTION__);
            }
        }
    }

    while(1) {
        vTaskDelay(10);
    }
}

void user_init(void)
{
    // Setup HW
    uart_set_baud(0, 115200);

    printf("SDK version:%s\n", sdk_system_get_sdk_version());

#ifdef I2C_CONNECTION
    i2c_init(I2C_BUS, SCL_PIN, SDA_PIN, I2C_FREQ_400K);
#endif

    while (ssd1306_init(&dev) != 0)
    {
        printf("%s: failed to init SSD1306 lcd\n", __func__);
        vTaskDelay(SECOND);
    }

    //ssd1306_set_whole_display_lighting(&dev, true); // WHITE
    ssd1306_set_whole_display_lighting(&dev, false); // BLACK
    vTaskDelay(SECOND);

    // Create user interface task
    xTaskCreate(ssd1306_task, "ssd1306_task", 1024, NULL, 2, NULL);
}
