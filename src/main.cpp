// pio device monitor --eol=CRLF --echo --filter send_on_enter
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h> // The Library
#include <LittleFS.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>
#include "camera.h"
#include "config.h"
#include "esp_camera.h"
#include "filesystem.h"
#include "gemini.h"
#include "serial.h"
#include "types.h"
#include "webserver.h"

#define LED_PIN 2
// Camera model selection - now defined in platformio.ini build_flags

// Global conversation history - circular buffer
std::array<Message, MAX_HISTORY> conversationHistory;
int historyHead = 0;
int historyCount = 0;
String url;

WiFiManager wm;
WiFiManagerParameter apiKeyParam("api_key", "Gemini API Key", GEMINI_API_KEY.c_str(), 100);
WiFiManagerParameter modelParam("model", "Gemini Model", GEMINI_MODEL.c_str(), 50);
WiFiManagerParameter urlParam("time_url", "Time API URL", TIME_API_URL.c_str(), 100);

void saveConfigCallback() {
  Serial.println("Saving config to LittleFS");
  File configFile = LittleFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return;
  }
  JsonDocument doc;
  doc["api_key"] = apiKeyParam.getValue();
  doc["model"] = modelParam.getValue();
  doc["time_url"] = urlParam.getValue();
  serializeJson(doc, configFile);
  configFile.close();
}

void loadConfig() {
  if (LittleFS.exists("/config.json")) {
    File configFile = LittleFS.open("/config.json", "r");
    if (configFile) {
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, configFile);
      if (!error) {
        GEMINI_API_KEY = doc["api_key"] | GEMINI_API_KEY;
        GEMINI_MODEL = doc["model"] | GEMINI_MODEL;
        TIME_API_URL = doc["time_url"] | TIME_API_URL;
      } else {
        Serial.println("Failed to parse config file");
      }
      configFile.close();
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  // Mount LittleFS
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed");
  } else {
    Serial.println("LittleFS mounted");
  }

  loadConfig();

  // Initialize camera

  if (!initCamera()) {
    Serial.println("Camera initialization failed!");
    while (true) {
      delay(1000);
    }
  }

  // WiFiManager setup
  wm.addParameter(&apiKeyParam);
  wm.addParameter(&modelParam);
  wm.addParameter(&urlParam);
  wm.setSaveParamsCallback(saveConfigCallback);

  bool res = wm.autoConnect("ESP32-AP");

  if (!res) {
    Serial.println("Failed to connect");
    ESP.restart();
  } else {
    Serial.println("Connected to WiFi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    // Update globals from params
    GEMINI_API_KEY = apiKeyParam.getValue();
    GEMINI_MODEL = modelParam.getValue();
    TIME_API_URL = urlParam.getValue();
  }

  // Construct URL with API key

  url = "https://generativelanguage.googleapis.com/v1beta/models/" +
        GEMINI_MODEL + ":generateContent?key=" + GEMINI_API_KEY;

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
