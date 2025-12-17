#include "serial.h"
#include "camera.h"
#include "gemini.h"
#include "types.h"
#include "filesystem.h"
#include "config.h"

#include <Arduino.h>
#include <esp_camera.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Preferences.h>

// TODO: make it some reserve as it will make multiple partitions so reserve some and if full reserve another
extern std::vector<Message> conversationHistory;
// 'buffer' is extern in serial.h
extern const char *URL;
extern const char *ssid;
extern const char *pass;

#define LED_PIN 2

void processBuffer(String buf) {
  Serial.println("\n--- Processing command ---");
  Serial.print("Received: '");
  Serial.print(buf);
  Serial.println("'");

  if (buf.startsWith("CAPTURE:")) {
    Serial.println("Step 1: Capture command detected");
    Serial.println("Step 2: Taking photo...");
    digitalWrite(LED_PIN, HIGH);

    camera_fb_t *fb = esp_camera_fb_get();
    if (fb) {
      Serial.println("Step 3: Camera frame buffer obtained");

      // Generate filename
      String imageFilename = getFileName();
      if (imageFilename == "chat_log.md") {
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
        Serial.println("Step 6: Image saved successfully");

        Serial.println("Step 7: Uploading image to Gemini...");
        if (uploadFileToGemini(fb->buf, fb->len, "image/jpeg", imageFilename)) {
          Serial.println("Step 8: Image uploaded to Gemini successfully");
        } else {
          Serial.println("Step 8: Failed to upload image to Gemini");
        }
      } else {
        Serial.println("Step 6: Failed to save image to filesystem");
      }

      esp_camera_fb_return(fb);
      Serial.println("Step 9: Camera frame buffer returned");

      delay(5000);
      digitalWrite(LED_PIN, LOW);
    } else {
      Serial.println("Step 3: Photo capture failed - camera buffer is null");
    }

  } else if (buf.startsWith("CHAT:")) {
    String userMessage = buf.substring(5);
    userMessage.trim();

    Serial.println("Step 1: Chat message detected");
    Serial.print("Step 2: Extracted message: '");
    Serial.print(userMessage);
    Serial.println("'");

    if (userMessage.length() > 0) {
      Message userMsg;
      userMsg.role = "user";
      userMsg.text = userMessage;
      conversationHistory.push_back(userMsg);

      Serial.println("Step 3: Added message to conversation history");
      Serial.println("Step 4: Sending to Gemini...");
      sendChatToGemini();
    } else {
      Serial.println("Empty chat message ignored");
    }

  } else if (buf.startsWith("RESET:")) {
    Serial.println("Reset command detected");
    Preferences prefs;
    prefs.begin("image", false);
    prefs.clear();
    prefs.end();
    Serial.println("Image counter reset");

  } else if (buf.startsWith("CLEAR:")) {
    Serial.println("Clear command detected");

    if (!LittleFS.begin(true)) {
      Serial.println("LittleFS mount failed");
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

  } else if (buf.length() > 0) {
    Serial.println("Unknown command");
  }

  Serial.println("--- Command processing complete ---\n");
}

void checkSerialChat(String webCommand) {
  static String serialBuffer = "";
  if (webCommand != "") { // there is web command
    processBuffer(webCommand);
  } else {
    while (Serial.available()) {
      char c = Serial.read();
      if (c == '\n' || c == '\r') {
        if (serialBuffer.length() > 0) {
          processBuffer(serialBuffer);
          serialBuffer = "";
        }
      } else {
        serialBuffer += c;
      }
    }
  }
}

