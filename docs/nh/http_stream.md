# Camera Stream JPEG
Extra feature implemented in **esp-drone-nh** board for camera streaming through http server using **FreeRTOS** event handler.

## Setup

In the `SDK Configuration editor`, go to the camera configuration and choose the following:

![ESP-Drone](../_static/camera_setup.png)

*Make sure that the WiFi setup is properly configured*

Once a device is connected to the board, a http server and listen to `GET requests` at `http://[board-ip]/stream.jpg`. When the request is triggered, it streams `QVGA JPEG` image from the camera.

## References
* Example of a simple http server implemented in ESP-IDF: https://github.com/espressif/esp-idf/tree/1067b28/examples/protocols/http_server/simple

* Another example for standalone HTTP camera streamer with ESP32 Camera with WiFi-STA implemented in ESP-IDF: https://github.com/NelsenEW/esp-stream-jpg


### Documentation
ESP-IDF Documentation for the implementation:
- https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/wifi.html#esp32-wi-fi-ap-general-scenario
- https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_http_server.html?highlight=http
- https://github.com/espressif/esp32-camera

Please refer to the [guide](../esp-drone-docs.pdf) to build and flash the board.