#include "MessageStore.h"
#include <MeshCore.h>

// File format version for future compatibility
#define MESSAGES_FILE_VERSION 1

MessageStore::MessageStore(DataStore* datastore)
  : _datastore(datastore), _message_count(0), _dirty(false) {
}

void MessageStore::begin() {
  load();
}

void MessageStore::load() {
  _message_count = 0;
  _dirty = false;

  File file = _datastore->openRead(MESSAGES_FILENAME);
  if (!file) {
    MESH_DEBUG_PRINTLN("No messages file found, starting fresh");
    return;
  }

  // Read version byte
  uint8_t version;
  if (file.read(&version, 1) != 1 || version != MESSAGES_FILE_VERSION) {
    MESH_DEBUG_PRINTLN("Invalid messages file version");
    file.close();
    return;
  }

  // Read message count
  uint8_t count;
  if (file.read(&count, 1) != 1) {
    MESH_DEBUG_PRINTLN("Failed to read message count");
    file.close();
    return;
  }

  // Read each message
  for (uint8_t i = 0; i < count && i < MAX_STORED_MESSAGES; i++) {
    StoredMessage* msg = &_messages[i];

    // Read fixed-size fields
    if (file.read((uint8_t*)&msg->timestamp, 4) != 4) break;
    if (file.read(msg->sender_id, 6) != 6) break;
    if (file.read((uint8_t*)msg->sender_name, MAX_SENDER_NAME_LEN) != MAX_SENDER_NAME_LEN) break;
    if (file.read(&msg->path_len, 1) != 1) break;
    if (file.read(&msg->flags, 1) != 1) break;
    if (file.read((uint8_t*)&msg->msg_len, 2) != 2) break;

    // Read variable-length message text
    if (msg->msg_len > MAX_MSG_TEXT_LEN) {
      MESH_DEBUG_PRINTLN("Message length exceeds maximum, truncating");
      msg->msg_len = MAX_MSG_TEXT_LEN;
    }
    if (file.read((uint8_t*)msg->msg, msg->msg_len) != msg->msg_len) break;
    msg->msg[msg->msg_len] = '\0'; // Ensure null termination

    _message_count++;
  }

  file.close();
  MESH_DEBUG_PRINTLN("Loaded %d messages from storage", _message_count);
}

void MessageStore::save() {
  if (!_dirty) {
    return; // No changes to save
  }

  // Count persistent messages (those with S: prefix)
  uint8_t persistent_count = 0;
  for (uint8_t i = 0; i < _message_count; i++) {
    if (_messages[i].flags & 0x04) { // Bit 2: persistent flag
      persistent_count++;
    }
  }

  MESH_DEBUG_PRINTLN("Saving %d persistent messages to storage (out of %d total)", persistent_count, _message_count);

  // Remove old file first
  _datastore->removeFile(MESSAGES_FILENAME);

#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  File file = _datastore->getPrimaryFS()->open(MESSAGES_FILENAME, FILE_O_WRITE);
#elif defined(RP2040_PLATFORM)
  File file = _datastore->getPrimaryFS()->open(MESSAGES_FILENAME, "w");
#else
  File file = _datastore->getPrimaryFS()->open(MESSAGES_FILENAME, "w", true);
#endif

  if (!file) {
    MESH_DEBUG_PRINTLN("Failed to open messages file for writing");
    return;
  }

  // Write version
  uint8_t version = MESSAGES_FILE_VERSION;
  file.write(&version, 1);

  // Write persistent message count
  file.write(&persistent_count, 1);

  // Write only persistent messages
  for (uint8_t i = 0; i < _message_count; i++) {
    const StoredMessage* msg = &_messages[i];

    // Only save persistent messages (those with S: prefix)
    if (!(msg->flags & 0x04)) {
      continue;
    }

    file.write((uint8_t*)&msg->timestamp, 4);
    file.write(msg->sender_id, 6);
    file.write((uint8_t*)msg->sender_name, MAX_SENDER_NAME_LEN);
    file.write(&msg->path_len, 1);
    file.write(&msg->flags, 1);
    file.write((uint8_t*)&msg->msg_len, 2);
    file.write((uint8_t*)msg->msg, msg->msg_len);
  }

  file.close();
  _dirty = false;
  MESH_DEBUG_PRINTLN("Persistent messages saved successfully");
}

bool MessageStore::addMessage(uint32_t timestamp, const uint8_t sender_id[6],
                               const char* sender_name, uint8_t path_len,
                               bool is_signed, const char* text, bool is_persistent) {
  // Check if we need to make space
  if (_message_count >= MAX_STORED_MESSAGES) {
    // Try to remove oldest non-persistent message first
    bool found_non_persistent = false;
    for (uint8_t i = 0; i < _message_count; i++) {
      if (!(_messages[i].flags & 0x04)) { // Not persistent
        MESH_DEBUG_PRINTLN("Message storage full, removing oldest non-persistent message");
        shiftMessagesLeft(i);
        _message_count--;
        found_non_persistent = true;
        break;
      }
    }

    // If no non-persistent message found, remove oldest message
    if (!found_non_persistent) {
      MESH_DEBUG_PRINTLN("Message storage full, removing oldest message");
      shiftMessagesLeft(0);
      _message_count--;
    }
  }

  // Add new message at the end
  StoredMessage* msg = &_messages[_message_count];
  msg->timestamp = timestamp;
  memcpy(msg->sender_id, sender_id, 6);

  strncpy(msg->sender_name, sender_name, MAX_SENDER_NAME_LEN - 1);
  msg->sender_name[MAX_SENDER_NAME_LEN - 1] = '\0';

  msg->path_len = path_len;
  msg->flags = 0; // Unread by default
  if (is_signed) {
    msg->flags |= 0x02; // Set signed flag
  }
  if (is_persistent) {
    msg->flags |= 0x04; // Set persistent flag (will be saved to disk)
  }

  // Copy message text
  int text_len = strlen(text);
  if (text_len > MAX_MSG_TEXT_LEN - 1) {
    text_len = MAX_MSG_TEXT_LEN - 1;
  }
  memcpy(msg->msg, text, text_len);
  msg->msg[text_len] = '\0';
  msg->msg_len = text_len;

  _message_count++;
  _dirty = true;

  MESH_DEBUG_PRINTLN("Added %s message from %s: %.30s...",
                     is_persistent ? "persistent" : "temporary", sender_name, text);
  return true;
}

bool MessageStore::deleteMessage(uint8_t idx) {
  if (idx >= _message_count) {
    return false;
  }

  MESH_DEBUG_PRINTLN("Deleting message at index %d", idx);
  shiftMessagesLeft(idx);
  _message_count--;
  _dirty = true;
  return true;
}

void MessageStore::shiftMessagesLeft(uint8_t from_idx) {
  for (uint8_t i = from_idx; i < _message_count - 1; i++) {
    _messages[i] = _messages[i + 1];
  }
}

void MessageStore::markAsRead(uint8_t idx) {
  if (idx >= _message_count) {
    return;
  }

  if (!(_messages[idx].flags & 0x01)) {
    _messages[idx].flags |= 0x01; // Set read flag
    _dirty = true;
  }
}

const StoredMessage* MessageStore::getMessage(uint8_t idx) const {
  if (idx >= _message_count) {
    return nullptr;
  }
  return &_messages[idx];
}

uint8_t MessageStore::getUnreadCount() const {
  uint8_t count = 0;
  for (uint8_t i = 0; i < _message_count; i++) {
    if (!(_messages[i].flags & 0x01)) {
      count++;
    }
  }
  return count;
}
