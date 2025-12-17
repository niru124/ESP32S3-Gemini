// pio device monitor --eol=CRLF --echo --filter send_on_enter
#include "camera.h"
#include "config.h"
#include "esp_camera.h"
#include "filesystem.h"
#include "gemini.h"
#include "serial.h"
#include "types.h"
#include "webserver.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h> // The Library
#include <LittleFS.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#define LED_PIN 2
// Camera model selection - now defined in platformio.ini build_flags

// Global conversation history
std::vector<Message> conversationHistory;
String url;

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  // Mount LittleFS
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed");
  } else {
    Serial.println("LittleFS mounted");
  }

  // Initialize camera

  if (!initCamera()) {
    Serial.println("Camera initialization failed!");
    while (true) {
      delay(1000);
    }
  }

  // Construct URL with API key

  url = "https://generativelanguage.googleapis.com/v1beta/models/" +
        String(GEMINI_MODEL) + ":generateContent?key=" + String(GEMINI_API_KEY);

  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);
  }

  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Start web server
  setupWebServer();

  // Check for chat messages via serial
  checkSerialChat();

  delay(100); // Small delay to prevent busy looping
}

void loop() {
  checkSerialChat();
  delay(100); // Small delay to prevent busy looping
}
