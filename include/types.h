#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>
#include <array>
#include <map>

// Structure to hold conversation messages
struct Message {
  String role;
  String text;
};

// MIME types map for file extensions
extern std::map<String, String> mimeTypes;

// Global conversation history - circular buffer
const int MAX_HISTORY = 20;
extern std::array<Message, MAX_HISTORY> conversationHistory;
extern int historyHead;
extern int historyCount;

#endif // TYPES_H