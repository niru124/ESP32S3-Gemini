#ifndef GEMINI_H
#define GEMINI_H

#include <Arduino.h>
#include "types.h"

extern String url;
extern String uploadedFileUri;
extern String uploadedFileMimeType;

bool ensureWiFiConnected();
void sendChatToGemini();
bool uploadFileToGemini(uint8_t* fileData, size_t fileSize, String mimeType, String displayName);
void save_history(String res, String user);

#endif // GEMINI_H