#include "wifi_esp32.h"
#define DEBUG_MODULE  "WIFI_UDP"
#include "debug_cf.h"

#define UDP_SERVER_PORT         2390
#define UDP_SERVER_BUFSIZE      128

static struct sockaddr_in6 source_addr; // Large enough for both IPv4 or IPv6
static int s_retry_num = 0;


static apsta_wifi_config_t apsta_wifi_config = {
        .sta = {
        .ssid = CONFIG_STA_SSID,//target ap ssid
        .password = CONFIG_STA_PWD,//target ap password
    },
    .ap = {
        .ssid = CONFIG_AP_SSID,
        .password = CONFIG_AP_PWD,
        .max_connection = CONFIG_MAX_STA_CONNECTION
,
        .authmode = WIFI_AUTH_WPA_WPA2_PSK,
    },
};


static char rx_buffer[UDP_SERVER_BUFSIZE];
static char tx_buffer[UDP_SERVER_BUFSIZE];
const int addr_family = (int)AF_INET;
const int ip_protocol = IPPROTO_IP;
static struct sockaddr_in dest_addr;
static int sock;

static xQueueHandle udpDataRx;
static xQueueHandle udpDataTx;
static UDPPacket inPacket;
static UDPPacket outPacket;

static bool isInit = false;
static bool isUDPInit = false;
static bool isUDPConnected = false;

static esp_err_t udp_server_create(void *arg);
static const char *TAG = "http_stream";

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

static uint8_t calculate_cksum(void *data, size_t len)
{
    unsigned char *c = data;
    int i;
    unsigned char cksum = 0;

    for (i = 0; i < len; i++) {
        cksum += *(c++);
    }

    return cksum;
}

static esp_err_t jpg_stream_httpd_handler(httpd_req_t *req){
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len;
    uint8_t * _jpg_buf;
    char * part_buf[64];
    static int64_t last_frame = 0;
    if(!last_frame) {
        last_frame = esp_timer_get_time();
    }

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if(res != ESP_OK){
        return res;
    }

    while(true){
        fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGE(TAG, "Camera capture failed");
            res = ESP_FAIL;
            break;
        }
        if(fb->format != PIXFORMAT_JPEG){
            bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
            if(!jpeg_converted){
                ESP_LOGE(TAG, "JPEG compression failed");
                esp_camera_fb_return(fb);
                res = ESP_FAIL;
            }
        } else {
            _jpg_buf_len = fb->len;
            _jpg_buf = fb->buf;
        }

        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }
        if(res == ESP_OK){
            size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);

            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }
        if(fb->format != PIXFORMAT_JPEG){
            free(_jpg_buf);
        }
        esp_camera_fb_return(fb);
        if(res != ESP_OK){
            break;
        }
        int64_t fr_end = esp_timer_get_time();
        int64_t frame_time = fr_end - last_frame;
        last_frame = fr_end;
        frame_time /= 1000;
        ESP_LOGI(TAG, "MJPG: %uKB %ums (%.1ffps)",
            (uint32_t)(_jpg_buf_len/1024),
            (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time);
    }

    last_frame = 0;
    return res;
}


static httpd_uri_t uri_handler_jpg = {
    .uri = "/stream.jpg",
    .method = HTTP_GET,
    .handler = jpg_stream_httpd_handler};

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.core_id = 0;

    // Start the httpd server
    DEBUG_PRINT_LOCAL("Starting HTTP stream server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK)
    {
      // Set URI handlers
      DEBUG_PRINT_LOCAL("Registering URI handlers");
      httpd_register_uri_handler(server, &uri_handler_jpg);
      return server;
    }
    DEBUG_PRINT_LOCAL("Error starting server!");
    return NULL;
}

static void stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    httpd_stop(server);
}


static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    httpd_handle_t *server = (httpd_handle_t *)arg;
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        DEBUG_PRINT_LOCAL("Attempting to connect to ssid \"%s\"", CONFIG_STA_SSID);
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP){
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        DEBUG_PRINT_LOCAL("got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        DEBUG_PRINT_LOCAL("Connected to ssid: \"%s\"", CONFIG_STA_SSID);
        s_retry_num = 0;

        /* Start the web server */
        if (*server == NULL){
            *server = start_webserver();
        }
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED){
  
        DEBUG_PRINT_LOCAL("Disconnected from wifi %s", CONFIG_STA_SSID);
        if (s_retry_num < CONFIG_STA_MAXIMUM_RETRY){
            esp_wifi_connect();
            s_retry_num++;
            DEBUG_PRINT_LOCAL("Retry to connect to wifi");
        } else{
            DEBUG_PRINT_LOCAL("Failed to connect to SSID %s", CONFIG_STA_SSID);
        }

        /* Stop the web server */
        if (*server){
            stop_webserver(*server);
            *server = NULL;
        }
    }
}

bool wifiTest(void)
{
    return isInit;
};

bool wifiGetDataBlocking(UDPPacket *in)
{
    /* command step - receive  02  from udp rx queue */
    while (xQueueReceive(udpDataRx, in, portMAX_DELAY) != pdTRUE) {
        vTaskDelay(1);
    }; // Don't return until we get some data on the UDP

    return true;
};

bool wifiSendData(uint32_t size, uint8_t *data)
{
    static UDPPacket outStage;
    outStage.size = size;
    memcpy(outStage.data, data, size);
    // Dont' block when sending
    return (xQueueSend(udpDataTx, &outStage, M2T(100)) == pdTRUE);
};

static esp_err_t udp_server_create(void *arg)
{   
    if (isUDPInit){
        return ESP_OK;
    }

    struct sockaddr_in *pdest_addr = &dest_addr;
    pdest_addr->sin_addr.s_addr = htonl(INADDR_ANY);
    pdest_addr->sin_family = AF_INET;
    pdest_addr->sin_port = htons(UDP_SERVER_PORT);

    sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
    if (sock < 0) {
        DEBUG_PRINT_LOCAL("Unable to create socket: errno %d", errno);
        return ESP_FAIL;
    }
    DEBUG_PRINT_LOCAL("Socket created");

    int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err < 0) {
        DEBUG_PRINT_LOCAL("Socket unable to bind: errno %d", errno);
    }
    DEBUG_PRINT_LOCAL("Socket bound, port %d", UDP_SERVER_PORT);

    isUDPInit = true;
    return ESP_OK;
}

static void udp_server_rx_task(void *pvParameters)
{
    uint8_t cksum = 0;
    socklen_t socklen = sizeof(source_addr);
    
    while (true) {
        if(isUDPInit == false) {
            vTaskDelay(20);
            continue;
        }
        int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);
        /* command step - receive  01 from Wi-Fi UDP */
        if (len < 0) {
            DEBUG_PRINT_LOCAL("recvfrom failed: errno %d", errno);
            break;
        } else if(len > WIFI_RX_TX_PACKET_SIZE - 4) {
            DEBUG_PRINT_LOCAL("Received data length = %d > 64", len);
        } else {
            //copy part of the UDP packet
            rx_buffer[len] = 0;// Null-terminate whatever we received and treat like a string...
            memcpy(inPacket.data, rx_buffer, len);
            cksum = inPacket.data[len - 1];
            //remove cksum, do not belong to CRTP
            inPacket.size = len - 1;
            //check packet
            if (cksum == calculate_cksum(inPacket.data, len - 1) && inPacket.size < 64){
                xQueueSend(udpDataRx, &inPacket, M2T(2));
                if(!isUDPConnected) isUDPConnected = true;
            }else{
                DEBUG_PRINT_LOCAL("udp packet cksum unmatched");
            }

#ifdef DEBUG_UDP_RECEIVE
            static char addr_str[128];
            inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
            DEBUG_PRINT_LOCAL("1.Received data size = %d  %02X \n cksum = %02X from %s", len, inPacket.data[0], cksum, addr_str);
            for (size_t i = 0; i < len; i++) {
                DEBUG_PRINT_LOCAL(" data[%d] = %02X ", i, inPacket.data[i]);
            }
#endif
        }
    }
}

static void udp_server_tx_task(void *pvParameters)
{
    while (TRUE) {
        if(isUDPInit == false) {
            vTaskDelay(20);
            continue;
        }
        if ((xQueueReceive(udpDataTx, &outPacket, 5) == pdTRUE) && isUDPConnected) {           
            memcpy(tx_buffer, outPacket.data, outPacket.size);       
            tx_buffer[outPacket.size] =  calculate_cksum(tx_buffer, outPacket.size);
            tx_buffer[outPacket.size + 1] = 0;
            int err = sendto(sock, tx_buffer, outPacket.size + 1, 0, (struct sockaddr *)&source_addr, sizeof(source_addr));
            if (err < 0) {
                DEBUG_PRINT_LOCAL("Error occurred during sending: errno %d", errno);
                continue;
            }
#ifdef DEBUG_UDP_SEND
            static char addr_str[128];
            inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
            DEBUG_PRINT_LOCAL("Send data to %s, size %d", addr_str, outPacket.size);
            for (size_t i = 0; i < outPacket.size + 1; i++) {
                DEBUG_PRINT_LOCAL(" data_send[%d] = %02X ", i, tx_buffer[i]);
            }
#endif
        }    
    }
}


static void wifi_ap_init()
{
    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();
    esp_netif_attach_wifi_ap(ap_netif);
    esp_netif_ip_info_t ip_info = {
        .ip.addr = ipaddr_addr("192.168.43.42"),
        .netmask.addr = ipaddr_addr("255.255.255.0"),
        .gw.addr      = ipaddr_addr("192.168.43.42"),
    };
    esp_netif_dhcps_stop(ap_netif);
    esp_netif_set_ip_info(ap_netif, &ip_info);
    esp_netif_dhcps_start(ap_netif);

}

static void wifi_sta_init()
{
    esp_netif_t *sta_if = esp_netif_create_default_wifi_sta();
    esp_netif_attach_wifi_station(sta_if);
} 

void wifiInit(void)
{
    static httpd_handle_t server = NULL;
    if (isInit) {
        return;
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_event_handler_register( WIFI_EVENT,
                                                ESP_EVENT_ANY_ID,
                                                &event_handler,
                                                &server));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,
                                                IP_EVENT_STA_GOT_IP,
                                                &event_handler,
                                                &server));

    wifi_ap_init();
    wifi_sta_init();

    
    esp_wifi_set_mode(WIFI_MODE_APSTA);
    esp_wifi_set_config(ESP_IF_WIFI_AP, (wifi_config_t*)&apsta_wifi_config.ap);
    esp_wifi_set_config(WIFI_IF_STA, (wifi_config_t*)&apsta_wifi_config.sta);
    esp_wifi_start();

    // This should probably be reduced to a CRTP packet size
    udpDataRx = xQueueCreate(5, sizeof(UDPPacket)); /* Buffer packets (max 64 bytes) */
    DEBUG_QUEUE_MONITOR_REGISTER(udpDataRx);
    udpDataTx = xQueueCreate(5, sizeof(UDPPacket)); /* Buffer packets (max 64 bytes) */
    DEBUG_QUEUE_MONITOR_REGISTER(udpDataTx);
    if (udp_server_create(NULL) == ESP_FAIL) {
        DEBUG_PRINT_LOCAL("UDP server create socket failed!!!");
    } else {
        DEBUG_PRINT_LOCAL("UDP server create socket succeed!!!");
    } 

    // server = start_webserver();
    // xTaskCreate(start_webserver, HTTP_STREAM_TASK_NAME, HTTP_STREAM_TASK_STACKSIZE, NULL, HTTP_STREAM_TASK_PRI, NULL);
    xTaskCreate(udp_server_tx_task, UDP_TX_TASK_NAME, UDP_TX_TASK_STACKSIZE, NULL, UDP_TX_TASK_PRI, NULL);
    xTaskCreate(udp_server_rx_task, UDP_RX_TASK_NAME, UDP_RX_TASK_STACKSIZE, NULL, UDP_RX_TASK_PRI, NULL);
    isInit = true;
}