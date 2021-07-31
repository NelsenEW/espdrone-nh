#include "wifi_esp32.h"
#define DEBUG_MODULE  "WIFI"
#include "debug_cf.h"

static int s_retry_num = 0;
static bool ap_connected = false;

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

static bool isInit = false;

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
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
        start_webserver();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED){
        /* Disconnect from the AP */
        ap_connected = true;
        esp_wifi_disconnect();
        
        /* Get the MAC and IP address of the new device */
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *) event_data;
        DEBUG_PRINT_LOCAL("station "MACSTR" join, AID=%d",
                          MAC2STR(event->mac), event->aid);
        
        /* Start the web server */
        start_webserver();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED){
        /* Stop the web server */
        stop_webserver();
        ap_connected = false;
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED){
        DEBUG_PRINT_LOCAL("Disconnected from wifi %s", CONFIG_STA_SSID);
        /* Dont attempt to reconnect if another ap is connected */
        if (ap_connected)
            return;
        if (s_retry_num < CONFIG_STA_MAXIMUM_RETRY){
            esp_wifi_connect();
            s_retry_num++;
            DEBUG_PRINT_LOCAL("Retry to connect to wifi");
        } else{
            DEBUG_PRINT_LOCAL("Failed to connect to SSID %s", CONFIG_STA_SSID);
        }
    }
}

bool wifiTest(void)
{
    return isInit;
};


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
    if (isInit) {
        return;
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK(esp_event_handler_register( WIFI_EVENT,
                                                ESP_EVENT_ANY_ID,
                                                &wifi_event_handler,
                                                NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,
                                                IP_EVENT_STA_GOT_IP,
                                                &wifi_event_handler,
                                                NULL));

    wifi_ap_init();
    wifi_sta_init();

    
    esp_wifi_set_mode(WIFI_MODE_APSTA);
    esp_wifi_set_config(ESP_IF_WIFI_AP, (wifi_config_t*)&apsta_wifi_config.ap);
    esp_wifi_set_config(WIFI_IF_STA, (wifi_config_t*)&apsta_wifi_config.sta);
    esp_wifi_start();

    if (udp_server_create(NULL) == ESP_FAIL) {
        DEBUG_PRINT_LOCAL("UDP server create socket failed!!!");
    } else {
        DEBUG_PRINT_LOCAL("UDP server create socket succeed!!!");
    } 

    // server = start_webserver();
    isInit = true;
}