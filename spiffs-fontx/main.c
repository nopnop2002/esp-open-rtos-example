#include <espressif/esp_common.h>
#include <esp/uart.h>
#include <FreeRTOS.h>
#include <task.h>
#include <esp_spiffs.h>

#include <fontx.h>

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

void fontTask(void *pvParameters)
{
    SPIFFS_Directory();

    // Open UTF8 to SJIS table
    spiffs_file fd = SPIFFS_open(&fs, Utf8Sjis, SPIFFS_O_RDONLY, 0);
    if (fd < 0) {
      printf("%s not found\n", Utf8Sjis);
      while (1) {
        vTaskDelay(2000 / portTICK_PERIOD_MS);
      }
    }

    char str_utf[MYBUFSZ+1];
    uint16_t sjis[128];
    int spos;

    uint8_t fonts[128];
    uint8_t pw;
    uint8_t ph;
    int i;

    /* 変換元文字列を作成（このソースはUTF-8で書かれている）*/
    strcpy(str_utf, "漢字TESTｶﾝｼﾞ");
    printf("%d\n", strlen(str_utf));

    /* 文字コード変換(UTF-8->SJIS) */
    spos = String2SJIS(fd, (unsigned char *)str_utf, strlen(str_utf), sjis, 128);
    /* フォントファイルの指定(お好みで) */
    FontxFile fx[2];
    InitFontx(fx,"ILGH16XB.FNT","ILGZ16XB.FNT"); // 16Dot Gothic
    //InitFontx(fx,"ILMH16XB.FNT","ILMZ16XB.FNT"); // 16Dot Mincyo
    //InitFontx(fx,"ILGH24XB.FNT","ILGZ24XB.FNT"); // 24Dot Gothic

    bool rc;
    for (i=0;i<spos;i++) {
      printf("sjis[%d]=0x%x\n",i,sjis[i]);
      rc = GetFontx(fx, sjis[i], fonts, &pw, &ph); // SJIS -> Fontパターン
      //printf("rc=%d pw=%d ph=%d\n",rc,pw,ph);
      if (!rc) continue;
      ShowFont(fonts, pw, ph);
#if 0
      uint8_t bitmap[128];
      Font2Bitmap(fonts, bitmap, pw, ph, 0);
      ShowBitmap(bitmap, pw, ph);
#endif
    }

    //DumpFontx(fx);
    while (1) {
      vTaskDelay(2000 / portTICK_PERIOD_MS);
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

    xTaskCreate(fontTask, "FontTask", 1024, NULL, 2, NULL);
}
