#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <Arduino.h>

#include <Arduino.h>

bool ensureWiFiConnected();

String getFileName();

void save_history(String res, String user);

void generateIndexHtml();

void deleteFile(const char *filename);

#endif // FILESYSTEM_H