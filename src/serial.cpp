#include "serial.h"
#include "camera.h"
#include "config.h"
#include "filesystem.h"
#include "gemini.h"
#include "types.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <esp_camera.h>

#define LED_PIN 2

// TODO: make it some reserve as it will make multiple partitions so reserve
// some and if full reserve another
extern std::vector<Message> conversationHistory;
// 'buffer' is extern in serial.h
extern const char *URL;
String buffer = "";
extern const char *ssid;
extern const char *pass;

void checkSerialChat() {
  while (Serial.available()) {
    char c = Serial.read();

    if (c == '\n' || c == '\r') {
      Serial.println("\n--- Processing command ---");
      Serial.print("Received: '");
      Serial.print(buffer);
      Serial.println("'");

      if (buffer.startsWith("RESET:")) {
        Serial.println("Reset command detected");
        Preferences prefs;
        prefs.begin("image", false);
        prefs.clear();
        prefs.end();
        Serial.println("Image counter reset to 0");
      } else if (buffer.startsWith("CLEAR:")) {
        Serial.println("Clear command detected");
        if (!LittleFS.begin(true)) {
          Serial.println("LittleFS Mount Failed");
        } else {
          File root = LittleFS.open("/");
          File file = root.openNextFile();
          while (file) {
            String filename = file.name();
            LittleFS.remove("/" + filename);
            Serial.println("Deleted: " + filename);
            file = root.openNextFile();
          }
          Serial.println("LittleFS cleared");
        }
      } else if (buffer.startsWith("CAPTURE:")) {
        Serial.println("Step 1: Capture command detected");
        Serial.println("Step 2: Taking photo...");
        digitalWrite(LED_PIN, HIGH);
        camera_fb_t *fb = esp_camera_fb_get();
        if (fb) {
          Serial.println("Step 3: Camera frame buffer obtained");
          // Save image to LittleFS
          String imageFilename = getFileName();
          if (imageFilename == "chat_log.md") {
            // Time API failed, use counter for unique filename
            Preferences prefs;
            prefs.begin("image", false);
            int count = prefs.getInt("count", 0);
            count++;
            prefs.putInt("count", count);
            prefs.end();
            imageFilename = "image_" + String(count) + ".jpg";
          } else {
            imageFilename.replace(".md", ".jpg");
          }
          Serial.print("Step 4: Generated filename: ");
          Serial.println(imageFilename);

          File imageFile = LittleFS.open("/" + imageFilename, "w");
          if (imageFile) {
            Serial.println("Step 5: Writing image to filesystem...");
            imageFile.write(fb->buf, fb->len);
            imageFile.close();
            Serial.println("Step 6: Image saved successfully: " +
                           imageFilename);

            // Upload image to Gemini
            Serial.println("Step 7: Uploading image to Gemini...");
            if (uploadFileToGemini(fb->buf, fb->len, "image/jpeg",
                                   imageFilename)) {
              Serial.println("Step 8: Image uploaded to Gemini successfully");
            } else {
              Serial.println("Step 8: Failed to upload image to Gemini");
            }
          } else {
            Serial.println("Step 6: Failed to save image to filesystem");
          }

          esp_camera_fb_return(fb);
          Serial.println("Step 9: Camera frame buffer returned");
          delay(5000); // Keep LED on for 5 seconds
          digitalWrite(LED_PIN, LOW);
        } else {
          Serial.println(
              "Step 3: Photo capture failed - camera frame buffer is null");
        }
      } else if (buffer.startsWith("CHAT:")) {
        // Treat any non-empty input as a chat message
        String userMessage = buffer;
        if (buffer.startsWith("CHAT:")) {
          userMessage = buffer.substring(strlen("CHAT:"));
        }
        userMessage.trim();

        Serial.println("Step 1: Chat message detected");
        Serial.print("Step 2: Extracted message: '");
        Serial.print(userMessage);
        Serial.println("' (length: " + String(userMessage.length()) + ")");

        if (userMessage.length() > 0) {
          Serial.println("Step 3: Processing chat message");
          Serial.println("--- New Chat Message ---");
          Serial.print("User: ");
          Serial.println(userMessage);

          // Add user message to history
          Message userMsg;
          userMsg.role = "user";
          userMsg.text = userMessage;
          conversationHistory.push_back(userMsg);

          Serial.println("Step 4: Added user message to conversation history");

          // Send to Gemini API
          Serial.println("Step 5: Sending to Gemini API...");
          sendChatToGemini();
          Serial.println("Step 6: Chat request sent to Gemini");
        } else {
          Serial.println("Step 3: Empty message - ignoring");
        }
      }

      Serial.println("--- Command processing complete ---\n");
      buffer = ""; // Clear buffer for next command
    } else {
      buffer += c; // Add character to buffer
    }
  }
}
