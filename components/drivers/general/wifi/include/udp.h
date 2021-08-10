#ifndef UDP_H_
#define UDP_H_
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
#include "stm32_legacy.h"

#define UDP_RX_TX_PACKET_SIZE   (64)

/* Structure used for in/out data via USB */
typedef struct
{
  uint8_t size;
  uint8_t data[UDP_RX_TX_PACKET_SIZE];
} UDPPacket;


/**
 * Get data from rx queue with timeout.
 * @param[out] c  Byte of data
 *
 * @return true if byte received, false if timout reached.
 */
bool udpGetDataBlocking(UDPPacket *in);

/**
 * Sends raw data using a lock. Should be used from
 * exception functions and for debugging when a lot of data
 * should be transfered.
 * @param[in] size  Number of bytes to send
 * @param[in] data  Pointer to data
 *
 * @note If WIFI Crtp link is activated this function does nothing
 */
bool udpSendData(uint32_t size, uint8_t* data);


/**
 * @brief Create the queue for the UDP Socket
 * 
 * @param arg 
 * @return esp_err_t 
 */
esp_err_t udp_server_create(void *arg);

#endif