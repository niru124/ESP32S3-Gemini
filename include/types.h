#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>
#include <array>

// Structure to hold conversation messages
struct Message {
  String role;
  String text;
};

// MIME type structure
struct MimeType {
  String ext;
  String type;
};

// MIME types array for file extensions
extern std::array<MimeType, 6> mimeTypes;

// Global conversation history - circular buffer
const int MAX_HISTORY = 20;
extern std::array<Message, MAX_HISTORY> conversationHistory;
extern int historyHead;
extern int historyCount;

#endif // TYPES_H