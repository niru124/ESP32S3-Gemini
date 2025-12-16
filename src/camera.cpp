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

  // Use PSRAM for better image quality - your board has 8MB PSRAM!
  config.frame_size = FRAMESIZE_VGA; // 640x480 - much better resolution
  config.jpeg_quality = 8; // Better quality (lower = better, range 0-63)
  config.fb_count = 2;     // Double buffering for better performance

  // Use PSRAM for frame buffers (much larger capacity)
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    Serial.println("Trying with smaller frame size...");

    // Fallback to smaller resolution if VGA fails
    config.frame_size =
        FRAMESIZE_QVGA; // 320x240 - still much better than QQVGA
    err = esp_camera_init(&config);
    if (err != ESP_OK) {
      Serial.printf("Camera init failed again with error 0x%x", err);
      Serial.println(
          "Camera hardware may not be connected or powered properly.");
      return false;
    }
  }

  // Configure sensor settings to fix green tint and improve image quality
  sensor_t *s = esp_camera_sensor_get();

  s->set_vflip(s, 1);      // flip it back
  s->set_hmirror(s, 1);    // mirror horizontally
  s->set_brightness(s, 2); // increase brightness
  s->set_contrast(s, 1);   // increase contrast
  s->set_saturation(s, -1); // reduce saturation to avoid green tint
  s->set_whitebal(s, 1);   // enable white balance
  s->set_awb_gain(s, 1);   // enable AWB gain
  s->set_wb_mode(s, 0);    // auto white balance
  s->set_gain_ctrl(s, 1);  // enable gain control
  s->set_exposure_ctrl(s, 1); // enable exposure control
  return true;
}

camera_fb_t *capturePhoto() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return nullptr;
  }
  return fb;
}
