

Forked from [ESP-Drone](https://github.com/espressif/esp-drone)

# ESPDrone-NH

## Introduction

**ESPDrone-NH** is an open source solution based on Espressif ESP32 Wi-Fi chip, which can be controlled by a mobile APP or gamepad over **Wi-Fi** connection. ESP-Drone comes with **simple hardware**, **clear and extensible code architecture**, and therefore this project can be used in **STEAM education** and other fields. The main code is ported from **Crazyflie** open source project with **GPL3.0** protocol.

<img src=./docs/_static/espdrone_nh.png alt="ESP-Drone" width=250 height=250>

For more information, please check the sections below:
* **Getting Started**: [Getting Started](<docs/nh/UAVIONICS Guide v1.0.pdf>)
* **Hardware Schematic**：[Hardware](docs/nh/Schematic_espdrone_nh_1.2.pdf)
* **iOS APP Source code**: [ESP-Drone-iOS](https://github.com/EspressifApps/ESP-Drone-iOS)
* **Android APP Source code**: [ESP-Drone-Android](https://github.com/EspressifApps/ESP-Drone-Android)

### Features

1. Stabilize Mode
2. Height-hold Mode
3. APP Control
4. EDclient Supported
5. [Camera streaming with httpd server](docs/nh/http_stream.md)

### Third Party Copyrighted Code

Additional third party copyrighted code is included under the following licenses.

| Component | License | Origin |Commit ID |
| :---:  | :---: | :---: |:---: |
| core/crazyflie | GPL3.0  |[Crazyflie](https://github.com/bitcraze/crazyflie-firmware) |a2a26abd53a5f328374877bfbcb7b25ed38d8111|
| lib/dsp_lib |  | [esp32-lin](https://github.com/whyengineer/esp32-lin/tree/master/components/dsp_lib) |6fa39f4cd5f7782b3a2a052767f0fb06be2378ff|
| lib/esp32-camera | | [esp32-cam](https://github.com/espressif/esp32-camera)| 488c308b79af2a66c285f4319b746943d6b2f893|

### THANKS

1. Thanks to Bitcraze for the great [Crazyflie project](https://www.bitcraze.io/%20).
2. Thanks to Espressif for the powerful [ESP-IDF framework](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s2/get-started/index.html).
3. Thanks to WhyEngineer for the useful [ESP-DSP lib](https://github.com/whyengineer/esp32-lin/tree/master/components/dsp_lib).
