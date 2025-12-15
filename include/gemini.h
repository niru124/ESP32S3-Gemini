#ifndef GEMINI_H
#define GEMINI_H

#include <Arduino.h>
#include "types.h"

extern String url;
extern String uploadedFileUri;

void sendChatToGemini();
bool uploadFileToGemini(uint8_t* fileData, size_t fileSize, String mimeType, String displayName);

#endif // GEMINI_H