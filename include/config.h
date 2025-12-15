#ifndef CONFIG_H
#define CONFIG_H

// WiFi credentials
extern const char *ssid;
extern const char *pass;

// Gemini API key
extern const char *GEMINI_API_KEY;

// Camera model selection
#ifndef CAMERA_MODEL_ESP32S3_EYE
#define CAMERA_MODEL_ESP32S3_EYE  // Fallback if not defined in build_flags
#endif

#endif // CONFIG_H