#ifndef WEBSERVER_H_ESP
#define WEBSERVER_H_ESP

#include <HTTPClient.h>
#include <ESPAsyncWebServer.h>

extern AsyncWebServer server;

void setupWebServer();

#endif // WEBSERVER_H_ESP