#ifndef CAMERA_H_
#define CAMERA_H_

#include <esp_log.h>
#include "esp_camera.h"


#ifdef CONFIG_TARGET_ESPDRONE_NH_V1
#define CONFIG_PWDN      -1
#define CONFIG_RESET     -1
#define CONFIG_XCLK      0
#define CONFIG_SDA       12
#define CONFIG_SCL       14
#define CONFIG_D7        34
#define CONFIG_D6        39
#define CONFIG_D5        36
#define CONFIG_D4        15
#define CONFIG_D3        4
#define CONFIG_D2        17
#define CONFIG_D1        16
#define CONFIG_D0        2
#define CONFIG_VSYNC     27
#define CONFIG_HREF      26
#define CONFIG_PCLK      5
#define CONFIG_XCLK_FREQ 20000000UL

#endif

esp_err_t cameraInit();
#endif