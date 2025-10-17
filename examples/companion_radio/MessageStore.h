#pragma once

#include <Arduino.h>
#include "DataStore.h"

#define MAX_STORED_MESSAGES 32
#define MAX_MSG_TEXT_LEN 200
#define MAX_SENDER_NAME_LEN 32

struct StoredMessage {
  uint32_t timestamp;           // When message was received
  uint8_t sender_id[6];          // 6-byte sender ID prefix
  char sender_name[MAX_SENDER_NAME_LEN]; // Sender display name
  uint8_t path_len;              // Path length (0xFF = direct)
  uint8_t flags;                 // Bit 0: read/unread, Bit 1: signed message, Bit 2: persistent (S: prefix)
  uint16_t msg_len;              // Length of message text
  char msg[MAX_MSG_TEXT_LEN];    // Message content (S: prefix included if persistent)
};

class MessageStore {
private:
  DataStore* _datastore;
  StoredMessage _messages[MAX_STORED_MESSAGES];
  uint8_t _message_count;
  bool _dirty;  // Needs to be saved

  const char* MESSAGES_FILENAME = "/messages.dat";

  void shiftMessagesLeft(uint8_t from_idx);

public:
  MessageStore(DataStore* datastore);

  void begin();
  void load();
  void save();

  // Add a new message (returns false if storage is full and couldn't make space)
  // is_persistent: true for messages with S: prefix (saved to disk), false for RAM-only
  bool addMessage(uint32_t timestamp, const uint8_t sender_id[6],
                  const char* sender_name, uint8_t path_len,
                  bool is_signed, const char* text, bool is_persistent);

  // Delete a message by index
  bool deleteMessage(uint8_t idx);

  // Mark message as read
  void markAsRead(uint8_t idx);

  // Get message by index
  const StoredMessage* getMessage(uint8_t idx) const;

  // Get total count
  uint8_t getCount() const { return _message_count; }

  // Get unread count
  uint8_t getUnreadCount() const;

  // Check if needs saving
  bool isDirty() const { return _dirty; }
};
