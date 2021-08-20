#ifndef WIFI_ESP32_H_
#define WIFI_ESP32_H_
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "http.h"
#include "udp.h"
#include "config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "ledseq.h"


#include <esp_log.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <esp_http_server.h>

#include  "queuemonitor.h"

#include "camera_setup.h"
#include "stm32_legacy.h"


typedef struct {
    wifi_sta_config_t sta;
    wifi_ap_config_t ap;
} apsta_wifi_config_t;

/**
 * Initialize the wifi.
 *
 * @note Initialize CRTP link only if USE_CRTP_WIFI is defined
 */
void wifiInit(void);

/**
 * Test the WIFI status.
 *
 * @return true if the WIFI is initialized
 */
bool wifiTest(void);


#endif