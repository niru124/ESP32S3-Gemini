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
  config.frame_size = FRAMESIZE_VGA;    // 640x480 - much better resolution
  config.jpeg_quality = 8;              // Better quality (lower = better, range 0-63)
  config.fb_count = 2;                  // Double buffering for better performance

  // Use PSRAM for frame buffers (much larger capacity)
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    Serial.println("Trying with smaller frame size...");

    // Fallback to smaller resolution if VGA fails
    config.frame_size = FRAMESIZE_QVGA;  // 320x240 - still much better than QQVGA
    err = esp_camera_init(&config);
    if (err != ESP_OK) {
      Serial.printf("Camera init failed again with error 0x%x", err);
      Serial.println("Camera hardware may not be connected or powered properly.");
      return false;
    }
  }

  // Configure sensor settings to fix green tint and improve image quality
  sensor_t *s = esp_camera_sensor_get();
  if (s != NULL) {
    // Fix green tint - adjust white balance and color correction
    s->set_whitebal(s, 1);        // Enable auto white balance
    s->set_awb_gain(s, 1);        // Enable AWB gain
    s->set_wb_mode(s, 0);         // Auto white balance mode

    // Color correction settings
    s->set_saturation(s, 0);      // Normal saturation (range: -2 to 2)
    s->set_brightness(s, 1);      // Slightly increase brightness (range: -2 to 2)
    s->set_contrast(s, 1);        // Slightly increase contrast (range: -2 to 2)

    // Exposure and gain control
    s->set_exposure_ctrl(s, 1);   // Enable auto exposure
    s->set_aec2(s, 1);           // Enable AEC DSP
    s->set_ae_level(s, 0);       // Auto exposure level
    s->set_aec_value(s, 300);    // Exposure value (lower = darker)

    // Additional settings for better image quality
    s->set_gain_ctrl(s, 1);      // Enable auto gain
    s->set_agc_gain(s, 0);       // Auto gain ceiling (0 = 2x, 1 = 4x, etc.)
    s->set_gainceiling(s, (gainceiling_t)0); // Gain ceiling

    // Denoise and other quality settings
    s->set_denoise(s, 1);        // Enable denoise
    s->set_quality(s, 8);        // JPEG quality (matches our config)

    Serial.println("Camera sensor configured for optimal image quality");
  } else {
    Serial.println("Warning: Could not get camera sensor handle");
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