#include "camera.h"
#include <Arduino.h>

bool initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // Your current ESP32-S3-DevKitC-1 has NO PSRAM, so use small frame size in internal RAM
  config.frame_size = FRAMESIZE_QQVGA;  // 160x120 - small but works without PSRAM
  config.jpeg_quality = 12;
  config.fb_count = 1;

  // Use internal DRAM instead of PSRAM (hardware has no PSRAM)
  config.fb_location = CAMERA_FB_IN_DRAM;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    Serial.println("Trying with even smaller frame size...");

    // If still fails, try the same frame size again (sometimes helps with timing)
    config.frame_size = FRAMESIZE_QQVGA;  // 160x120
    err = esp_camera_init(&config);
    if (err != ESP_OK) {
      Serial.printf("Camera init failed again with error 0x%x", err);
      Serial.println("Camera hardware may not be connected or powered properly.");
      return false;
    }
  }

  return true;
}

camera_fb_t* capturePhoto() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return nullptr;
  }
  return fb;
}