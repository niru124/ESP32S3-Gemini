#include "gemini.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include "types.h"

extern String url;
extern std::array<Message, MAX_HISTORY> conversationHistory;
extern int historyHead;
extern int historyCount;
String uploadedFileUri = ""; // Store the uploaded file URI
String uploadedFileMimeType = ""; // Store the uploaded file MIME type

const int MAX_WINDOW_SIZE = 10; // Sliding window size

// Define and initialize MIME types array
std::array<MimeType, 6> mimeTypes = {{
  {"jpg", "image/jpeg"},
  {"jpeg", "image/jpeg"},
  {"mp4", "video/mp4"},
  {"mp3", "audio/mpeg"},
  {"wav", "audio/wav"},
  {"pdf", "application/pdf"}
}};

// Function to get MIME type from file extension
String getMimeType(String filename) {
  int dotIndex = filename.lastIndexOf('.');
  if (dotIndex == -1) return "application/octet-stream";
  String ext = filename.substring(dotIndex + 1);
  ext.toLowerCase();

  // Lookup in MIME types array
  for (const auto& mt : mimeTypes) {
    if (mt.ext == ext) return mt.type;
  }
  return "application/octet-stream";
}

String summarizeConversation(const std::vector<Message>& discarded) {
  String prompt = "Summarize the following conversation history in a concise way, focusing on key facts, decisions, and user requests. Preserve any URLs mentioned:\n";
  for (const auto& msg : discarded) {
    prompt += msg.role + ": " + msg.text + "\n";
  }

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");

  JsonDocument doc;
  JsonArray contents = doc["contents"].to<JsonArray>();
  JsonObject contentObj = contents.add<JsonObject>();
  contentObj["role"] = "user";
  JsonArray parts = contentObj["parts"].to<JsonArray>();
  JsonObject partObj = parts.add<JsonObject>();
  partObj["text"] = prompt;

  String httpRequestData;
  serializeJson(doc, httpRequestData);
  http.setTimeout(30000);

  int httpResponseCode = http.POST(httpRequestData);
  if (httpResponseCode > 0) {
    String response = http.getString();
    JsonDocument responseDoc;
    DeserializationError error = deserializeJson(responseDoc, response);
    if (!error && responseDoc["candidates"].is<JsonArray>()) {
      String summary = responseDoc["candidates"][0]["content"]["parts"][0]["text"];
      http.end();
      return summary;
    }
  }
  http.end();
  return "Summary not available.";
}

void manageConversationHistory() {
  if (historyCount <= MAX_WINDOW_SIZE) return;

  std::vector<Message> discarded;
  for (int i = 0; i < historyCount - MAX_WINDOW_SIZE; i++) {
    discarded.push_back(conversationHistory[(historyHead + i) % MAX_HISTORY]);
  }

  String summary = summarizeConversation(discarded);

  Message summaryMsg = {"system", "Summary of earlier conversation: " + summary};
  conversationHistory[0] = summaryMsg;
  int newCount = 1;

  int startIdx = (historyHead + (historyCount - MAX_WINDOW_SIZE)) % MAX_HISTORY;
  for (int i = 0; i < MAX_WINDOW_SIZE; i++) {
    conversationHistory[newCount++] = conversationHistory[(startIdx + i) % MAX_HISTORY];
  }

  historyHead = 0;
  historyCount = newCount;
}

void sendChatToGemini() {
  // Manage history before sending aka summerizing if needed based on sliding window
  manageConversationHistory();

  WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    Serial.print("Connecting to Gemini API...");
    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");

    // Build JSON with full conversation history
    JsonDocument doc;
    JsonArray contents = doc["contents"].to<JsonArray>();

    for (int i = 0; i < historyCount; i++) {
      JsonObject contentObj = contents.add<JsonObject>();
      contentObj["role"] = conversationHistory[(historyHead + i) % MAX_HISTORY].role;
      JsonArray parts = contentObj["parts"].to<JsonArray>();
      JsonObject partObj = parts.add<JsonObject>();
      partObj["text"] = conversationHistory[(historyHead + i) % MAX_HISTORY].text;
    }

    // Add the uploaded file as a separate content if we have one
    if (uploadedFileUri != "") {
      JsonObject contentObj = contents.add<JsonObject>();
      contentObj["role"] = "user";
      JsonArray parts = contentObj["parts"].to<JsonArray>();
      JsonObject filePart = parts.add<JsonObject>();
      JsonObject fileData = filePart["fileData"].to<JsonObject>();
      fileData["mimeType"] = uploadedFileMimeType;
      fileData["fileUri"] = uploadedFileUri;
    }

    String httpRequestData;
    serializeJson(doc, httpRequestData);
    //NOTE:
    // Set longer timeout for Gemini API (30 seconds)
    http.setTimeout(30000);

    Serial.println("Sending request...");
    Serial.printf("Request payload size: %d bytes\n", httpRequestData.length());
    Serial.println("JSON payload:");
    Serial.println(httpRequestData);
    Serial.println("Connecting to Gemini API...");

    int httpResponseCode = http.POST(httpRequestData);
    Serial.printf("Chat response code: %d\n", httpResponseCode);

    if (httpResponseCode > 0) {
      String response = http.getString();

      // Parse Gemini response
      JsonDocument responseDoc;
      DeserializationError error = deserializeJson(responseDoc, response);

      if (!error && responseDoc["candidates"].is<JsonArray>()) {
        String modelText =
            responseDoc["candidates"][0]["content"]["parts"][0]["text"];

        Serial.print("Gemini: ");
        Serial.println(modelText);

         // Add model response to conversation history
          Message modelMsg = {"model", modelText};
          conversationHistory[(historyHead + historyCount) % MAX_HISTORY] = modelMsg;
          historyCount++;

          Serial.print("Conversation history now has ");
          Serial.print(historyCount);
          Serial.println(" messages.\n");

          // Save the query and response to filesystem
          if (historyCount >= 2) {
            String userQuery = conversationHistory[(historyHead + historyCount - 2) % MAX_HISTORY].text;
            save_history(modelText, userQuery);
          }
      } else {
        Serial.println("Error parsing Gemini response");
        Serial.println(response);
      }
    } else {
      Serial.print("HTTP Error: ");
      Serial.println(httpResponseCode);
      Serial.println(http.errorToString(httpResponseCode));
    }

    http.end();
}

bool uploadFileToGemini(uint8_t* fileData, size_t fileSize, String filename) {
  String mimeType = getMimeType(filename);
  String displayName = filename; // Use filename as display name

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;

  String baseUrl = "https://generativelanguage.googleapis.com";
  String uploadUrl = baseUrl + "/upload/v1beta/files?key=" + String(GEMINI_API_KEY);

  // Step 1: Initiate resumable upload
  http.begin(client, uploadUrl);
  http.addHeader("X-Goog-Upload-Protocol", "resumable");
  http.addHeader("X-Goog-Upload-Command", "start");
  http.addHeader("X-Goog-Upload-Header-Content-Length", String(fileSize));
  http.addHeader("X-Goog-Upload-Header-Content-Type", mimeType);
  http.addHeader("Content-Type", "application/json");

  // Collect specific headers for debugging
  const char* headerKeys[] = {"X-Goog-Upload-URL", "Location", "x-goog-upload-url", "upload-url"};
  http.collectHeaders(headerKeys, 4);

  String jsonPayload = "{\"file\": {\"display_name\": \"" + displayName + "\"}}";
  Serial.println("Initiating upload with payload: " + jsonPayload);
  int httpResponseCode = http.POST(jsonPayload);

  Serial.printf("Upload initiation response code: %d\n", httpResponseCode);

  if (httpResponseCode != 200) {
    Serial.printf("Upload initiation failed: %d\n", httpResponseCode);
    String responseBody = http.getString();
    Serial.println("Response body: " + responseBody);
    http.end();
    return false;
  }

  // Try different possible header names for the upload URL
  String uploadUrlHeader = "";

  // Check various possible header names
  const char* possibleHeaders[] = {"X-Goog-Upload-URL", "x-goog-upload-url", "Location", "upload-url"};
  for (int i = 0; i < 4; i++) {
    uploadUrlHeader = http.header(possibleHeaders[i]);
    if (uploadUrlHeader != "") {
      Serial.println("Found upload URL in header '" + String(possibleHeaders[i]) + "': " + uploadUrlHeader);
      break;
    }
  }

  if (uploadUrlHeader == "") {
    Serial.println("No upload URL found in any expected headers");
    // Print all response headers for debugging
    Serial.println("All response headers:");
    for (int i = 0; i < http.headers(); i++) {
      Serial.printf("Header %d: %s = %s\n", i, http.headerName(i).c_str(), http.header(i).c_str());
    }

    // Also print the response body to see if URL is there
    String responseBody = http.getString();
    Serial.println("Response body: " + responseBody);
    http.end();
    return false;
  }

  Serial.println("Upload URL obtained: " + uploadUrlHeader);
  http.end();

  // Step 2: Upload the actual file data
  http.begin(client, uploadUrlHeader);
  http.addHeader("Content-Length", String(fileSize));
  http.addHeader("X-Goog-Upload-Offset", "0");
  http.addHeader("X-Goog-Upload-Command", "upload, finalize");

  httpResponseCode = http.POST(fileData, fileSize);

  if (httpResponseCode != 200) {
    Serial.printf("File upload failed: %d\n", httpResponseCode);
    http.end();
    return false;
  }

  String response = http.getString();
  http.end();

  // Parse the response to get file URI
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, response);
  if (error) {
    Serial.println("Failed to parse upload response");
    return false;
  }

  String fileUri = doc["file"]["uri"];
  if (fileUri == "") {
    Serial.println("No file URI in response");
    return false;
  }

  uploadedFileUri = fileUri;
  uploadedFileMimeType = mimeType;
  Serial.println("File uploaded successfully. URI: " + uploadedFileUri + ", MIME: " + mimeType);
  return true;
}
