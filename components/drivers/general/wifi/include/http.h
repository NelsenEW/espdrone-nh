#ifndef HTTP_H_
#define HTTP_H_

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
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



#include <esp_log.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <esp_http_server.h>

#include  "queuemonitor.h"

#include "camera_setup.h"
/**
 * @brief start the webserver
 * 
 */
void start_webserver();

/**
 * @brief stop the webserver
 * 
 */
void stop_webserver();

#endif 