#include "webserver.h"
#include "filesystem.h"
#include "serial.h"
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <Preferences.h>

AsyncWebServer server(80);

void handleDelete(AsyncWebServerRequest *request) {
  if (request->hasParam("filename", true)) {
    String filename = request->getParam("filename", true)->value();
    deleteFile(filename.c_str());
    request->send(200, "text/plain", "Deleted");
  } else {
    request->send(400, "text/plain", "Missing filename");
  }
}

void setupWebServer() {
  // Serve static files from LittleFS
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  // Endpoint to list images
  server.on("/api/images", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "[";
    File root = LittleFS.open("/");
    File file = root.openNextFile();
    bool first = true;
    while (file) {
      String filename = file.name();
      if (filename.endsWith(".jpg")) {
        if (!first)
          json += ",";
        json += "\"" + filename + "\"";
        first = false;
      }
      file = root.openNextFile();
    }
    json += "]";
    request->send(200, "application/json", json);
  });

  // Endpoint to list .md files
  server.on("/api/files", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "[";
    File root = LittleFS.open("/");
    File file = root.openNextFile();
    bool first = true;
    while (file) {
      String filename = file.name();
      if (filename.endsWith(".md")) {
        if (!first)
          json += ",";
        json += "\"" + filename + "\"";
        first = false;
      }
      file = root.openNextFile();
    }
    json += "]";
    request->send(200, "application/json", json);
  });

  // Endpoint to delete a file
  server.on("/delete", HTTP_POST, handleDelete);

  // Add a simple test endpoint
  server.on("/test", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Web server is running!");
  });

  server.begin();
  Serial.println("Web server started on port 80");
}
