// pio device monitor --eol=CRLF --echo --filter send_on_enter
#include "camera.h"
#include "config.h"
#include "esp_camera.h"
#include "gemini.h"
#include "types.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h> // The Library
#include <WiFi.h>
#include <WiFiClientSecure.h>

// Camera model selection - now defined in platformio.ini build_flags
#ifndef CAMERA_MODEL_ESP32S3_EYE
#define CAMERA_MODEL_ESP32S3_EYE // Fallback if not defined in build_flags
#endif

// Global conversation history
std::vector<Message> conversationHistory;

void checkSerialChat();

void setup() {
  Serial.begin(115200);
  delay(2000);

  // Initialize camera
  if (!initCamera()) {
    Serial.println("Camera initialization failed!");
    while (true) {
      delay(1000);
    }
  }
  Serial.println("Camera initialized successfully!");

  // Construct URL with API key
  url = "https://generativelanguage.googleapis.com/v1beta/models/"
        "gemini-2.5-flash:generateContent?key=" +
        String(GEMINI_API_KEY);

  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  Serial.println("\n=== Gemini Chat with Camera Demo ===");
  Serial.println("Commands:");
  Serial.println("  CAPTURE: - Take a photo and upload to Gemini");
  Serial.println("  CHAT:your message here - Chat with Gemini (will include "
                 "uploaded image if available)");
  Serial.println("  TEST: - Test basic Gemini API connection (no image)");
  Serial.println("Example: CAPTURE:");
  Serial.println("Example: CHAT:Describe this image");
  Serial.println("Example: TEST:");
  Serial.println(
      "The conversation history will be maintained automatically.\n");

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi is connected. Ready for chat!");
  }
}

void loop() {
  // Check for chat messages via serial
  checkSerialChat();
  delay(100); // Small delay to prevent busy looping
}

void checkSerialChat() {
  static String buffer = ""; // Static buffer to accumulate characters

  while (Serial.available() > 0) {
    char c = Serial.read();

    if (c == '\n' || c == '\r') {
      // End of line received
      buffer.trim();

      // Debug: show what we received
      Serial.print("Received command: '");
      Serial.print(buffer);
      Serial.println("'");

      if (buffer.startsWith("CAPTURE:")) {
        Serial.println("\n--- Capturing Photo ---");
        camera_fb_t *fb = capturePhoto();
        if (fb) {
          Serial.printf("Captured photo: %d bytes\n", fb->len);
          // Upload to Gemini
          if (uploadFileToGemini(fb->buf, fb->len, "image/jpeg",
                                 "esp32_camera_photo")) {
            Serial.println("Photo uploaded successfully!");
          } else {
            Serial.println("Photo upload failed!");
          }
          esp_camera_fb_return(fb);
        } else {
          Serial.println("Photo capture failed!");
        }
      } else if (buffer.startsWith("CHAT:")) {
        String userMessage = buffer.substring(5);
        userMessage.trim();

        Serial.print("Extracted message: '");
        Serial.print(userMessage);
        Serial.println("' (length: ");
        Serial.print(userMessage.length());
        Serial.println(")");

        if (userMessage.length() > 0) {
          Serial.println("\n--- New Chat Message ---");
          Serial.print("User: ");
          Serial.println(userMessage);

          // Add user message to history
          Message userMsg;
          userMsg.role = "user";
          userMsg.text = userMessage;
          conversationHistory.push_back(userMsg);

          // Send to Gemini API
          sendChatToGemini();
        } else {
          Serial.println("Empty message after CHAT:");
        }
      } else if (buffer.length() > 0) {
        Serial.println("Command doesn't start with CHAT:");
      }

      buffer = ""; // Clear buffer for next command
    } else {
      buffer += c; // Add character to buffer
    }
  }
}
