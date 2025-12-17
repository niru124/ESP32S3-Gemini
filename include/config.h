#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Gemini API key
extern String GEMINI_API_KEY;
// Gemini Model
extern String GEMINI_MODEL;

// Time API URL
extern String TIME_API_URL;
extern String url;

// Camera model selection
#ifndef CAMERA_MODEL_ESP32S3_EYE
#define CAMERA_MODEL_ESP32S3_EYE // Fallback if not defined in build_flags
#endif

#endif // CONFIG_H
