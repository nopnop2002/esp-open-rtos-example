#include <espressif/esp_common.h>
#include <esp/uart.h>
#include <FreeRTOS.h>
#include <task.h>
#include <string.h>
#include <esp_spiffs.h>
#include <ssd1306/ssd1306.h>

#include <fontx.h>

/* Remove this line if your display connected by SPI */
#define I2C_CONNECTION

#ifdef I2C_CONNECTION
    #include <i2c/i2c.h>
#endif
#include "fonts/fonts.h"

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

#define DEFAULT_FONT FONT_FACE_TERMINUS_6X12_ISO8859_1

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


const font_info_t *font = NULL; // current font

#define SECOND (1000 / portTICK_PERIOD_MS)

#define MYBUFSZ 128

static void SPIFFS_Directory() {
    spiffs_DIR d;
    struct spiffs_dirent e;
    struct spiffs_dirent *pe = &e;

    SPIFFS_opendir(&fs, "/", &d);
    printf("SPIFFS Directory List\n");
    while ((pe = SPIFFS_readdir(&d, pe))) {
      printf("%s [%04x] size:%i\n", pe->name, pe->obj_id, pe->size);
    }
    SPIFFS_closedir(&d);
}

int ssd1306_draw_font(const ssd1306_t *dev, uint8_t *fb, uint8_t x, uint8_t y, 
	uint8_t *fonts, uint8_t pw, uint8_t ph,
        ssd1306_color_t foreground, ssd1306_color_t background)
{
    uint8_t i, j;
    int err;
    int index = 0;

    for (j = 0; j < ph; ++j) {
        for (i = 0; i < pw; ++i) {
//printf("j=%d i=%d fonts[%d]=%x\n",j,i,index+(i/8),fonts[index+(i/8)]);
            if (fonts[index+(i/8)] & (0x80 >> (i % 8))) {
//printf("ON x+i=%d y+j=%d\n",x+i,y+j);
              err = ssd1306_draw_pixel(dev, fb, x + i, y + j, foreground);
            } else {
//printf("OFF x+i=%d y+j=%d\n",x+i,y+j);
              switch (background) {
                case OLED_COLOR_TRANSPARENT:
                  // Not drawing for transparent background
                  break;
                case OLED_COLOR_WHITE:
                case OLED_COLOR_BLACK:
                  err = ssd1306_draw_pixel(dev, fb, x + i, y + j, background);
                  break;
                case OLED_COLOR_INVERT:
                  // I don't know why I need invert background
                  break;
              }
            }
            if (err) return -ERANGE;
        }
        index = index + (pw+7)/8;
    }
    return pw;
} 

static void ssd1306_task(void *pvParameters)
{
    TickType_t nowTick;
    printf("%s: Started user interface task\n", __FUNCTION__);
    SPIFFS_Directory();
    font = font_builtin_fonts[7];

    // Open UTF8 to SJIS table
    spiffs_file fd = SPIFFS_open(&fs, Utf8Sjis, SPIFFS_O_RDONLY, 0);
    if (fd < 0) {
      printf("%s not found\n", Utf8Sjis);
      while (1) {
        vTaskDelay(2000 / portTICK_PERIOD_MS);
      }
    }

    ssd1306_set_whole_display_lighting(&dev, true);
    vTaskDelay(SECOND);
    ssd1306_set_whole_display_lighting(&dev, false);
    vTaskDelay(SECOND);
    ssd1306_fill_rectangle(&dev, buffer, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT/2, OLED_COLOR_BLACK);

    char str_utf[MYBUFSZ+1];
    uint16_t sjis[128];
    int sjisLen;

    uint8_t fonts[128];
    //uint8_t bitmap[128];
    uint8_t pw;
    uint8_t ph;
    int x, y;
    int i;

    /* 変換元文字列を作成（このソースはUTF-8で書かれている）*/
    strcpy(str_utf, "漢字TESTｶﾝｼﾞ");
    printf("%d\n", strlen(str_utf));

    /* 文字コード変換(UTF-8->SJIS) */
    sjisLen = String2SJIS(fd, (unsigned char *)str_utf, strlen(str_utf), sjis, 128);
    /* フォントファイルの指定(お好みで) */
    FontxFile fx[2];
    InitFontx(fx,"ILGH16XB.FNT","ILGZ16XB.FNT"); // 16Dot Gothic
    //InitFontx(fx,"ILMH16XB.FNT","ILMZ16XB.FNT"); // 16Dot Mincyo
    //InitFontx(fx,"ILGH24XB.FNT","ILGZ24XB.FNT"); // 24Dot Gothic

    bool rc;
    x = 0;
    y = 0;
    for (i=0;i<sjisLen;i++) {
      printf("sjis[%d]=0x%x\n",i,sjis[i]);
      rc = GetFontx(fx, sjis[i], fonts, &pw, &ph); // SJIS -> Fontパターン
      printf("rc=%d pw=%d ph=%d\n",rc,pw,ph);
      if (!rc) continue;
      ShowFont(fonts, pw, ph);

      ssd1306_draw_font(&dev, buffer, x, y, fonts, pw, ph, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
      x = x + pw;

      //Font2Bitmap(fonts, bitmap, pw, ph, 0);
      //ShowBitmap(bitmap, pw, ph);
    }

    char text[20];
    while (1) {
        nowTick = xTaskGetTickCount();
        sprintf(text, "Tick: %d   ", nowTick);
        ssd1306_draw_string(&dev, buffer, font_builtin_fonts[DEFAULT_FONT], 0, 45, text, OLED_COLOR_WHITE, OLED_COLOR_BLACK);

        if (ssd1306_load_frame_buffer(&dev, buffer)) {
          printf("%s: error while loading framebuffer into SSD1306\n", __func__);
          while(1) {
            vTaskDelay(2 * SECOND);
            printf("%s: error loop\n", __FUNCTION__);
          }
        }
    }

}

void user_init(void)
{
    uart_set_baud(0, 115200);
    printf("SDK version:%s\n", sdk_system_get_sdk_version());

    /* Mount SPIFFS */
    esp_spiffs_init();

    if (esp_spiffs_mount() != SPIFFS_OK) {
      printf("SPIFFS mount Fail\n");
      while (1) {
        vTaskDelay(2000 / portTICK_PERIOD_MS);
      }
    }
    printf("SPIFFS mount OK\n");


#ifdef I2C_CONNECTION
    i2c_init(I2C_BUS, SCL_PIN, SDA_PIN, I2C_FREQ_400K);
#endif

    while (ssd1306_init(&dev) != 0) {
        printf("%s: failed to init SSD1306 lcd\n", __func__);
        vTaskDelay(SECOND);
    }

    xTaskCreate(ssd1306_task, "ssd1306_task", 1024, NULL, 2, NULL);

}
