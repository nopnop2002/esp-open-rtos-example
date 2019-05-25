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


static void ssd1306_print_text_left(uint8_t * buf, int page, char * str, int strlen, bool flag) {
    int pos = page * 128;
    for(int i=0;i<strlen;i++) {
        int ch = str[i];
    	memcpy(&buf[pos], font8x8_basic_tr[ch],8);
        if (flag) ssd1306_invert_text(&buf[pos], 8);
        pos = pos + 8;
    }
}

static void ssd1306_print_text_right(uint8_t * buf, int page, char * str, int strlen, bool flag) {
    int pos = page * 128;
    int margin = 16 - strlen;
    pos = pos + (margin * 8);
    for(int i=0;i<strlen;i++) {
        int ch = str[i];
    	memcpy(&buf[pos], font8x8_basic_tr[ch],8);
        if (flag) ssd1306_invert_text(&buf[pos], 8);
        pos = pos + 8;
    }
}

static void ssd1306_print_text_center(uint8_t * buf, int page, char * str, int strlen, bool flag) {
    int pos = page * 128;
    int margin = (16 - strlen) / 2;
    pos = pos + (margin * 8);
    for(int i=0;i<strlen;i++) {
        int ch = str[i];
    	memcpy(&buf[pos], font8x8_basic_tr[ch],8);
        if (flag) ssd1306_invert_text(&buf[pos], 8);
        pos = pos + 8;
    }
}

static void ssd1306_clear_text(uint8_t * buf, int page, bool flag) {
    char str[16];
    memset(str, 0x20, 16);
    ssd1306_print_text_left(buf, page, str, 16, flag);
}

static void ssd1306_task(void *pvParameters)
{
    printf("%s: Started user interface task\n", __FUNCTION__);
    vTaskDelay(1000/portTICK_PERIOD_MS);

#if 0
    ssd1306_fill_rectangle(&dev, buffer, 0, 0, DISPLAY_WIDTH,
			 DISPLAY_HEIGHT/2, OLED_COLOR_WHITE);
#endif
    ssd1306_print_text_center(buffer, 0, "0123456789", 10, false);
    ssd1306_print_text_left(buffer, 1, "0123456789", 10, false);
    ssd1306_print_text_right(buffer, 2, "0123456789", 10, false);

    char text[20];
    while(1) {
        TickType_t nowTick = xTaskGetTickCount();
	sprintf(text, "Tick:%d", nowTick);
	ssd1306_clear_text(buffer, 4, false);
	ssd1306_print_text_center(buffer, 4, text, strlen(text), false);
	ssd1306_clear_text(buffer, 5, true);
	ssd1306_print_text_center(buffer, 5, text, strlen(text), true);
        ssd1306_invert_frame(DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, 0, buffer, invert_buffer);
        if (ssd1306_load_frame_buffer(&dev, buffer)) {
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
