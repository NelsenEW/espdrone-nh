idf_component_register(SRCS "wifi_esp32.c"
                       INCLUDE_DIRS "." "include"
                     REQUIRES crazyflie utils platform config nvs_flash esp_http_server)