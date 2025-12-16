#include "filesystem.h"
#include <LittleFS.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "config.h"

// Extern declarations for global variables
extern const char* ssid;
extern const char* pass;
extern String url;

// Helper function to ensure WiFi connection
bool ensureWiFiConnected() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. Attempting to reconnect...");
    WiFi.begin(ssid, pass);
    int retryCount = 0;
    while (WiFi.status() != WL_CONNECTED && retryCount < 20) { // Retry for 10 seconds
      delay(500);
      Serial.print(".");
      retryCount++;
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nWiFi reconnected!");
      return true;
    }
    else {
      Serial.println("\nFailed to reconnect to WiFi.");
      return false;
    }
  }
  return true;
}

String getFileName() {
  if (!ensureWiFiConnected()) {
    return "chat_log.md"; // Fallback if WiFi not connected
  }

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.begin(URL);
  int httpResponseCode = http.GET();
  int retryCount = 0;
  while (httpResponseCode <= 0 && retryCount < 5) {
    delay(1000);
    Serial.print("Retrying time API...");
    httpResponseCode = http.GET();
    retryCount++;
  }
  if (httpResponseCode > 0) {
    Serial.println("Successfully got current time");
    String raw_data = http.getString();

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, raw_data);
    if (error) {
      Serial.println("Failed to parse time JSON");
      return "chat_log.md"; // Fallback
    }
    String datetime = doc["datetime"];
    // Parse "2025-12-16T01:43:47.179537+05:30"
    int tIndex = datetime.indexOf('T');
    String date = datetime.substring(0, tIndex); // "2025-12-16"
    String timeFull = datetime.substring(tIndex + 1); // "01:43:47.179537+05:30"
    String time = timeFull.substring(0, 5); // "01:43"
    String filename = date + "_" + time + ".md";
    filename.replace(":", "-"); // Replace : with - for filename safety
    return filename;
  }
  else {
    Serial.println("Failed to get time from API after retries");
    return "chat_log.md"; // Fallback
  }
}

void save_history(String res, String user) {
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS Mount Failed");
    return;
  }

  Serial.println("Writing to file...");

  // Open for appending("a") - add "/" prefix for LittleFS path
  String filename = getFileName();
  File file = LittleFS.open("/" + filename, "a");

  if (!file) {
    Serial.println("Failed to open file for writing");
  }
  else {
    file.println(user);
    file.println("---");
    file.println(res);
    file.println("---");
    file.close();
    Serial.println("Write complete.");
  }
}

void deleteFile(const char *filename) {
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS Mount Failed");
    return;
  }

  // Add "/" prefix to filename for LittleFS path
  String filePath = "/" + String(filename);

  if (LittleFS.remove(filePath)) {
    Serial.println("File deleted: " + String(filename));
  } else {
    Serial.println("Failed to delete file: " + String(filename));
  }
}


