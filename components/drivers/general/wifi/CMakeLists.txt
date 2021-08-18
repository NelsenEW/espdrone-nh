idf_component_register(SRCS "http.c" "wifi_esp32.c" "udp.c"
                       INCLUDE_DIRS "." "include"
                     REQUIRES espdrone utils platform config nvs_flash esp_http_server)