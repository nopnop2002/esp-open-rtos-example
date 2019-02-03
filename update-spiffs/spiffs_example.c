#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "ssid_config.h"

#include "fcntl.h"
#include "unistd.h"

#include "spiffs.h"
#include "esp_spiffs.h"
#include "jsmn.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

/**
 * This example shows the default SPIFFS configuration when SPIFFS is
 * configured in compile-time (SPIFFS_SINGLETON = 1).
 *
 * To configure SPIFFS in run-time uncomment SPIFFS_SINGLETON in the Makefile
 * and replace the commented esp_spiffs_init in the code below.
 *
 */

#define TELNET_PORT 23 // Telnet port
#define RECEIVE_PORT 8100 // Receive port
#define UDP_PORT 8200 // Broadcast port

#define FILE_NAME "init.json"

/* Remove this line if read by SPIFFS */
#define USE_POSIX

/* Enable this line if wait for telnet client input */
//#define ENABLE_PROMPT

char Gbuf[1024];

SemaphoreHandle_t xSemaphore1;
SemaphoreHandle_t xSemaphore2;
SemaphoreHandle_t xSemaphore3;
SemaphoreHandle_t xMutex;
QueueHandle_t xQueue;
TimerHandle_t timerHandle;

UBaseType_t uxQueueLength = 10;
UBaseType_t uxItemSize = 64;

static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
  if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
    strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
    return 0;
  }
  return -1;
}

static void jsonp(int dst, char * JSON_STRING) {
  int i;
  int r;
  jsmn_parser p;
  jsmntok_t t[128]; /* We expect no more than 128 tokens */

  jsmn_init(&p);
  r = jsmn_parse(&p, JSON_STRING, strlen(JSON_STRING), t, sizeof(t)/sizeof(t[0]));
  if (r < 0) {
    sprintf(Gbuf, "Failed to parse JSON: %d\n", r);
    if (dst == 0) printf("%s", Gbuf);
    if (dst != 0) lwip_write(dst, Gbuf, strlen(Gbuf));
  }

  /* Assume the top-level element is an object */
  if (r < 1 || t[0].type != JSMN_OBJECT) {
    sprintf(Gbuf, "Object expected\n");
    if (dst == 0) printf("%s", Gbuf);
    if (dst != 0) lwip_write(dst, Gbuf, strlen(Gbuf));
  }

  /* Loop over all keys of the root object */
  for (i = 1; i < r; i++) {
    if (jsoneq(JSON_STRING, &t[i], "user") == 0) {
	/* We may use strndup() to fetch string value */
	sprintf(Gbuf, "- User: %.*s\n", t[i+1].end-t[i+1].start,
		JSON_STRING + t[i+1].start);
        if (dst == 0) printf("%s", Gbuf);
        if (dst != 0) lwip_write(dst, Gbuf, strlen(Gbuf));
	i++;
    } else if (jsoneq(JSON_STRING, &t[i], "admin") == 0) {
	/* We may additionally check if the value is either "true" or "false" */
	sprintf(Gbuf, "- Admin: %.*s\n", t[i+1].end-t[i+1].start,
		JSON_STRING + t[i+1].start);
        if (dst == 0) printf("%s", Gbuf);
        if (dst != 0) lwip_write(dst, Gbuf, strlen(Gbuf));
	i++;
    } else if (jsoneq(JSON_STRING, &t[i], "uid") == 0) {
	/* We may want to do strtol() here to get numeric value */
	sprintf(Gbuf, "- UID: %.*s\n", t[i+1].end-t[i+1].start,
		JSON_STRING + t[i+1].start);
        if (dst == 0) printf("%s", Gbuf);
        if (dst != 0) lwip_write(dst, Gbuf, strlen(Gbuf));
	i++;
    } else if (jsoneq(JSON_STRING, &t[i], "groups") == 0) {
	int j;
	sprintf(Gbuf, "- Groups:\n");
        if (dst == 0) printf("%s", Gbuf);
        if (dst != 0) lwip_write(dst, Gbuf, strlen(Gbuf));
	if (t[i+1].type != JSMN_ARRAY) {
	  continue; /* We expect groups to be an array of strings */
	}
	for (j = 0; j < t[i+1].size; j++) {
	  jsmntok_t *g = &t[i+j+2];
	  sprintf(Gbuf, "  * %.*s\n", g->end - g->start, JSON_STRING + g->start);
          if (dst == 0) printf("%s", Gbuf);
          if (dst != 0) lwip_write(dst, Gbuf, strlen(Gbuf));
	}
	i += t[i+1].size + 1;
   } else {
	sprintf(Gbuf, "Unexpected key: %.*s\n", t[i].end-t[i].start,
	JSON_STRING + t[i].start);
        if (dst == 0) printf("%s", Gbuf);
        if (dst != 0) lwip_write(dst, Gbuf, strlen(Gbuf));
   }
}

}

#if defined(USE_POSIX)
static void example_read_file_posix(int dst, char * fileName)
{
    const int buf_size = 0xFF;
    uint8_t buf[buf_size];

    int fd = open(fileName, O_RDONLY);
    if (fd < 0) {
        printf("Error opening file\n");
        return;
    }

    int read_bytes = read(fd, buf, buf_size);
    sprintf(Gbuf, "Read %d bytes\n", read_bytes);
    if (dst == 0) printf("%s", Gbuf);
    //if (dst != 0) lwip_write(dst, Gbuf, strlen(Gbuf));

    buf[read_bytes] = '\0';    // zero terminate string
    sprintf(Gbuf, "Data: %s\n", buf);
    if (dst == 0) printf("%s", Gbuf);
    //if (dst != 0) lwip_write(dst, Gbuf, strlen(Gbuf));

    if (dst == 0) jsonp(dst, (char *)buf);
    if (dst != 0) jsonp(dst, (char *)buf);

    close(fd);
}
#endif

#if !defined(USE_POSIX)
static void example_read_file_spiffs(int dst, char * fileName)
{
    const int buf_size = 0xFF;
    uint8_t buf[buf_size];

    spiffs_file fd = SPIFFS_open(&fs, fileName, SPIFFS_RDONLY, 0);
    if (fd < 0) {
        printf("Error opening file\n");
        return;
    }

    int read_bytes = SPIFFS_read(&fs, fd, buf, buf_size);
    sprintf(Gbuf, "Read %d bytes\n", read_bytes);
    if (dst == 0) printf("%s", Gbuf);
    //if (dst != 0) lwip_write(dst, Gbuf, strlen(Gbuf));

    buf[read_bytes] = '\0';    // zero terminate string
    sprintf(Gbuf, "Data: %s\n", buf);
    if (dst == 0) printf("%s", Gbuf);
    //if (dst != 0) lwip_write(dst, Gbuf, strlen(Gbuf));

    if (dst == 0) jsonp(dst, (char *)buf);
    if (dst != 0) jsonp(dst, (char *)buf);

    SPIFFS_close(&fs, fd);
}
#endif

static void example_write_file_posix(char * fileName, uint8_t * buf, int blen)
{
    int fd = open(fileName, O_WRONLY|O_CREAT, 0);
    if (fd < 0) {
        printf("Error opening file\n");
        return;
    }

    int written = write(fd, buf, blen);
    printf("Written %d bytes\n", written);

    close(fd);
}

static void example_fs_info(int dst)
{
    uint32_t total, used;
    SPIFFS_info(&fs, &total, &used);
    sprintf(Gbuf, "SPIFFS Information:\n");
    if (dst == 0) printf("%s", Gbuf);
    if (dst != 0) lwip_write(dst, Gbuf, strlen(Gbuf));
    sprintf(Gbuf, "Total: %d bytes, used: %d bytes\n", total, used);
    if (dst == 0) printf("%s", Gbuf);
    if (dst != 0) lwip_write(dst, Gbuf, strlen(Gbuf));
}

static void example_fs_dir(int dst)
{
    spiffs_DIR d;
    struct spiffs_dirent e;
    struct spiffs_dirent *pe = &e;

    SPIFFS_opendir(&fs, "/", &d);
    sprintf(Gbuf, "Directory Information:\n");
    if (dst == 0) printf("%s", Gbuf);
    if (dst != 0) lwip_write(dst, Gbuf, strlen(Gbuf));
    while ((pe = SPIFFS_readdir(&d, pe))) {
      sprintf(Gbuf,"%s [%04x] size:%i\n", pe->name, pe->obj_id, pe->size);
      if (dst == 0) printf("%s", Gbuf);
      if (dst != 0) lwip_write(dst, Gbuf, strlen(Gbuf));
    }
    SPIFFS_closedir(&d);
}

static void showNetworkInfo() {
  uint8_t hwaddr[6];
  if (sdk_wifi_get_macaddr(STATION_IF, hwaddr)) {
    printf("[%s] MAC address="MACSTR"\n",pcTaskGetName(0),MAC2STR(hwaddr));
  }
  struct ip_info info;
  if (sdk_wifi_get_ip_info(STATION_IF, &info)) {
    printf("[%s] IP address="IPSTR"\n",pcTaskGetName(0),IP2STR(&info.ip));
    printf("[%s] Netmask="IPSTR"\n",pcTaskGetName(0),IP2STR(&info.netmask));
    printf("[%s] Gateway="IPSTR"\n",pcTaskGetName(0),IP2STR(&info.gw));
  }
}

// Timer call back
static void timer_cb(TimerHandle_t xTimer)
{
  char buffer[uxItemSize];
  TickType_t nowTick;
  nowTick = xTaskGetTickCount();
  printf("[%s:%d] Start\n",pcTaskGetName(0),nowTick);

  struct ip_info info;
  if (sdk_wifi_get_ip_info(STATION_IF, &info)) {
    printf("[%s:%d] IP address="IPSTR"\n",pcTaskGetName(0),nowTick,IP2STR(&info.ip));
    //sprintf(buffer,"Hello World!! %d",nowTick);
    sprintf(buffer,"Connect to "IPSTR,IP2STR(&info.ip));
    xQueueSend(xQueue, &buffer, 0);
  }
}


//
// WiFi Task
// This task is the task which watches a connection to the AP.
//
void task1(void *pvParameters)
{
  TickType_t nowTick;
  nowTick = xTaskGetTickCount();
  printf("[%s:%d] Start\n",pcTaskGetName(0),nowTick);

  /* Wait until we have joined AP and are assigned an IP */
  int wifiStatus;
  while (1) {
    wifiStatus = sdk_wifi_station_get_connect_status();
    nowTick = xTaskGetTickCount();
    printf("[%s:%d] wifiStatus=%d\n",pcTaskGetName(0),nowTick,wifiStatus);
    //if (sdk_wifi_station_get_connect_status() == STATION_GOT_IP) break;
    if (wifiStatus == STATION_GOT_IP) break;
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  nowTick = xTaskGetTickCount();
  printf("[%s:%d] Connected to AP\n",pcTaskGetName(0),nowTick);
  showNetworkInfo();

  /* Now start task2 */
  xSemaphoreGive(xSemaphore1);

  /* Now start task3 */
  xSemaphoreGive(xSemaphore2);

  /* Now start task4 */
  xSemaphoreGive(xSemaphore3);

  // Create & Start Timer
  timerHandle = xTimerCreate("Trigger", 5000/portTICK_PERIOD_MS, pdTRUE, NULL, timer_cb);
  nowTick = xTaskGetTickCount();
  if (timerHandle != NULL) {
    if (xTimerStart(timerHandle, 0) != pdPASS) {
      printf("[%s:%d] Unable to start Timer ...\n",pcTaskGetName(0),nowTick);
    } else {
      printf("[%s:%d] Success to start Timer ...\n",pcTaskGetName(0),nowTick);
    }
  } else {
    printf("[%s:%d] Unable to create Timer ...\n",pcTaskGetName(0),nowTick);
  }

  while(1) {
    vTaskDelay(1000);
  }
}

//
// Telnet Task
// This task is the task which accepts a connection from portnumber 23.
//
void task2(void *pvParameters)
{
  TickType_t nowTick;
  nowTick = xTaskGetTickCount();
  printf("[%s:%d] Start\n",pcTaskGetName(0),nowTick);

  /* wait for Semaphore */
  xSemaphoreTake(xSemaphore1, portMAX_DELAY);
  nowTick = xTaskGetTickCount();
  printf("[%s:%d] Take\n",pcTaskGetName(0),nowTick);

  /* set up address to connect to */
  struct sockaddr_in srcAddr;
  struct sockaddr_in dstAddr;
  memset(&srcAddr, 0, sizeof(srcAddr));
  srcAddr.sin_family = AF_INET;
  srcAddr.sin_port = htons(TELNET_PORT);
  srcAddr.sin_addr.s_addr = INADDR_ANY;

  /* create the socket */
  int srcSocket;
  int dstSocket;
  socklen_t dstAddrSize;
  int ret;
  int numrcv;

  srcSocket = lwip_socket(AF_INET, SOCK_STREAM, 0);
  LWIP_ASSERT("srcSocket >= 0", srcSocket >= 0);

  /* bind socket */
  ret = lwip_bind(srcSocket, (struct sockaddr *)&srcAddr, sizeof(srcAddr));
  /* should succeed */
  LWIP_ASSERT("ret == 0", ret == 0);

  /* listen socket */
  ret = lwip_listen(srcSocket, 5);
  /* should succeed */
  LWIP_ASSERT("ret == 0", ret == 0);

#if SPIFFS_SINGLETON == 1
  esp_spiffs_init();
#else
  // for run-time configuration when SPIFFS_SINGLETON = 0
  esp_spiffs_init(0x200000, 0x10000);
#endif

  if (esp_spiffs_mount() != SPIFFS_OK) {
      printf("Error mount SPIFFS\n");
  }

  while(1) {
    // 接続の受付け
    // Reception of a connection
    //printf("[%s:%d] 接続を待っています クライアントプログラムを動かして下さい\n",pcTaskGetName(0),nowTick);
    printf("[%s:%d] Wait for client connection....\n",pcTaskGetName(0),nowTick);
    dstAddrSize = sizeof(dstAddr);
    dstSocket = lwip_accept(srcSocket, (struct sockaddr *)&dstAddr, &dstAddrSize);
    printf("[%s:%d] dstSocket=%d\n",pcTaskGetName(0),nowTick,dstSocket);
    nowTick = xTaskGetTickCount();
    //printf("[%s:%d] %s から接続を受けました\n",pcTaskGetName(0),nowTick,inet_ntoa(dstAddr.sin_addr));
    printf("[%s:%d] A connection was received from %s\n",pcTaskGetName(0),nowTick,inet_ntoa(dstAddr.sin_addr));

    example_fs_info(0);
    //example_fs_info(dstSocket);

    example_fs_dir(0);
    //example_fs_dir(dstSocket);

    // Take Mutex, Enter critical section
    xSemaphoreTake(xMutex, portMAX_DELAY);

#if defined(USE_POSIX)
    example_read_file_posix(0, FILE_NAME);
    example_read_file_posix(dstSocket, FILE_NAME);
#else
    example_read_file_spiffs(0, FILE_NAME);
    example_read_file_spiffs(dstSocket, FILE_NAME);
#endif

// for DEBUG
#if 0
    uint8_t bbuf[] = "{\"user\": \"charie\", \"admin\": false, \"uid\": 1300}";
    example_write_file_posix(FILE_NAME, bbuf, sizeof(bbuf));

#if defined(USE_POSIX)
    example_read_file_posix(0, FILE_NAME);
    example_read_file_posix(dstSocket, FILE_NAME);
#else
    example_read_file_spiffs(0, FILE_NAME);
    example_read_file_spiffs(dstSocket, FILE_NAME);
#endif
#endif
// for DEBUG

    // Give Mutex, Leave critical section
    xSemaphoreGive(xMutex);

    // クライアントがSocketをクローズしてからこちらもクローズする
    // When a client closes a socket, this side also closes a socket.
    while(1) {

#if defined(ENABLE_PROMPT)
      /* write prompt */
      strcpy(Gbuf,"\r\n>");
      ret = lwip_write(dstSocket, Gbuf, strlen(Gbuf));
#endif

      /* read something */
      memset(Gbuf,0,sizeof(Gbuf));
      numrcv = lwip_read(dstSocket, Gbuf, 1024);
      nowTick = xTaskGetTickCount();
      printf("[%s:%d] numrcv=%d\n",pcTaskGetName(0),nowTick,numrcv);
      if(numrcv ==0 || numrcv ==-1 ){ // client close socket
        lwip_close(dstSocket); break;
      }

#if defined(ENABLE_PROMPT)
      printf("Recv=[%s]\n",Gbuf);
      if(strncmp(Gbuf,"exit",4) == 0){
        lwip_close(dstSocket); break;
      }
#endif
    } // end while

  } // wnd wihile
}

//
// Receiver Task
// This task is the task which accepts a connection from portnumber 8100.
//
void task3(void *pvParameters)
{
  TickType_t nowTick;
  nowTick = xTaskGetTickCount();
  printf("[%s:%d] Start\n",pcTaskGetName(0),nowTick);

  /* wait for Semaphore */
  xSemaphoreTake(xSemaphore2, portMAX_DELAY);
  nowTick = xTaskGetTickCount();
  printf("[%s:%d] Take\n",pcTaskGetName(0),nowTick);

  /* set up address to connect to */
  struct sockaddr_in srcAddr;
  struct sockaddr_in dstAddr;
  memset(&srcAddr, 0, sizeof(srcAddr));
  srcAddr.sin_family = AF_INET;
  srcAddr.sin_port = htons(RECEIVE_PORT);
  srcAddr.sin_addr.s_addr = INADDR_ANY;

  /* create the socket */
  int srcSocket;
  int dstSocket;
  socklen_t dstAddrSize;
  int ret;
  int numrcv;

  srcSocket = lwip_socket(AF_INET, SOCK_STREAM, 0);
  LWIP_ASSERT("srcSocket >= 0", srcSocket >= 0);

  /* bind socket */
  ret = lwip_bind(srcSocket, (struct sockaddr *)&srcAddr, sizeof(srcAddr));
  /* should succeed */
  LWIP_ASSERT("ret == 0", ret == 0);

  /* listen socket */
  ret = lwip_listen(srcSocket, 5);
  /* should succeed */
  LWIP_ASSERT("ret == 0", ret == 0);

  //char buf[1024];
  uint8_t buf[1024];
  while(1) {
    // 接続の受付け
    // Reception of a connection
    //printf("[%s:%d] 接続を待っています クライアントプログラムを動かして下さい\n",pcTaskGetName(0),nowTick);
    printf("[%s:%d] Wait for client connection....\n",pcTaskGetName(0),nowTick);
    dstAddrSize = sizeof(dstAddr);
    dstSocket = lwip_accept(srcSocket, (struct sockaddr *)&dstAddr, &dstAddrSize);
    nowTick = xTaskGetTickCount();
    printf("[%s:%d] %s から接続を受けました\n",pcTaskGetName(0),nowTick,inet_ntoa(dstAddr.sin_addr));

    // クライアントがSocketをクローズしてからこちらもクローズする
    // When a client closes a socket, this side also closes a socket.
    while(1) {
      /* read something */
      memset(buf,0,sizeof(buf));
      numrcv = lwip_read(dstSocket, buf, 1024);
      nowTick = xTaskGetTickCount();
      printf("[%s:%d] numrcv=%d\n",pcTaskGetName(0),nowTick,numrcv);
      if(numrcv ==0 || numrcv ==-1 ){ // client close socket
        lwip_close(dstSocket); break;
      }
      printf("numrecv=%d buf=[%s]\n",numrcv, buf);

      // Take Mutex, Enter critical section
      xSemaphoreTake(xMutex, portMAX_DELAY);

      // Write file
      example_write_file_posix(FILE_NAME, buf, numrcv);

      // Give Mutex, Leave critical section
      xSemaphoreGive(xMutex);
    } // end while
  } // end for


  /* close (never come here) */
  ret = lwip_close(srcSocket);
  LWIP_ASSERT("ret == 0", ret == 0);
  vTaskDelete( NULL );

}

//
// Bradcast Client Task
// This task is the task which broadcast my IP to portnumber 8200.
//
void task4(void *pvParameters)
{
  TickType_t nowTick;
  nowTick = xTaskGetTickCount();
  printf("[%s:%d] Start\n",pcTaskGetName(0),nowTick);

  /* wait for Semaphore */
  xSemaphoreTake(xSemaphore3, portMAX_DELAY);
  nowTick = xTaskGetTickCount();
  printf("[%s:%d] Take\n",pcTaskGetName(0),nowTick);

  /* set up address to connect to */
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(UDP_PORT);
  addr.sin_addr.s_addr = htonl(INADDR_BROADCAST); /* send message to 255.255.255.255 */


  /* create the socket */
  int fd;
  int ret;
  fd = lwip_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP ); // Create a UDP socket.
  LWIP_ASSERT("fd >= 0", fd >= 0);

  char buffer[uxItemSize];
  int buflen;
  while(1) {
    if(xQueueReceive(xQueue, &buffer, portMAX_DELAY)) {
       nowTick = xTaskGetTickCount();
       printf("[%s:%d] buffer=[%s]\n",pcTaskGetName(0),nowTick,buffer);
       buflen = strlen(buffer);
       ret = lwip_sendto(fd, buffer, buflen, 0, (struct sockaddr *)&addr, sizeof(addr));
       LWIP_ASSERT("ret == buflen", ret == buflen);
       nowTick = xTaskGetTickCount();
       printf("[%s:%d] lwip_sendto ret=%d\n",pcTaskGetName(0),nowTick,ret);
    } else {
       nowTick = xTaskGetTickCount();
       printf("[%s:%d] xQueueReceive fail\n",pcTaskGetName(0),nowTick);
       break;
    }
  }

  /* close */
  ret = lwip_close(fd);
  LWIP_ASSERT("ret == 0", ret == 0);
  vTaskDelete( NULL );

}

void user_init(void)
{
  uart_set_baud(0, 115200);
  printf("SDK version:%s\n", sdk_system_get_sdk_version());

  struct sdk_station_config config = {
      .ssid = WIFI_SSID,
      .password = WIFI_PASS,
  };

  // Required to call wifi_set_opmode before station_set_config.
  sdk_wifi_set_opmode(STATION_MODE);
  sdk_wifi_station_set_config(&config);

  /* Create Semaphore */
  xSemaphore1 = xSemaphoreCreateBinary();
  xSemaphore2 = xSemaphoreCreateBinary();
  xSemaphore3 = xSemaphoreCreateBinary();

  /* Create Mutex */
  xMutex = xSemaphoreCreateMutex();

  /* Create Queue */
  xQueue = xQueueCreate(uxQueueLength, uxItemSize);

  /* Check everything was created. */
  configASSERT( xSemaphore1 );
  configASSERT( xSemaphore2 );
  configASSERT( xSemaphore3 );
  configASSERT( xMutex );
  configASSERT( xQueue );

#if 0
  // Create Timer
  timerHandle = xTimerCreate("Trigger", 5000/portTICK_PERIOD_MS, pdTRUE, NULL, timer_cb);
  if (timerHandle != NULL) {
    if (xTimerStart(timerHandle, 0) != pdPASS) {
      printf("%s: Unable to start Timer ...\n", __FUNCTION__);
    } else {
      printf("%s: Success to start Timer ...\n", __FUNCTION__);
    }
  } else {
    printf("%s: Unable to create Timer ...\n", __FUNCTION__);
  }
#endif

  /* Start task */
  xTaskCreate(task1, "WiFi", 256, NULL, 2, NULL);
  xTaskCreate(task2, "Telnet", 1024, NULL, 2, NULL);
  xTaskCreate(task3, "Receiver", 1024, NULL, 2, NULL);
  xTaskCreate(task4, "Broadcast", 1024, NULL, 2, NULL);
}
