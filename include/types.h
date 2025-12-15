#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>
#include <vector>

// Structure to hold conversation messages
struct Message {
  String role;
  String text;
};

// Global conversation history
extern std::vector<Message> conversationHistory;

#endif // TYPES_H