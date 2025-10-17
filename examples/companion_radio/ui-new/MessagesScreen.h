#pragma once

#include <helpers/ui/UIScreen.h>
#include <helpers/ui/DisplayDriver.h>
#include "../MessageStore.h"

class UITask;

class MessagesScreen : public UIScreen {
private:
  MessageStore* _msg_store;
  UITask* _task;

  enum ViewMode {
    LIST_VIEW,      // Showing list of messages
    MESSAGE_VIEW,   // Reading a specific message
    MENU_VIEW       // Showing delete menu
  };

  ViewMode _mode;
  uint8_t _selected_idx;        // Currently selected message in list
  uint8_t _scroll_offset;       // Scroll position in list
  uint8_t _message_scroll;      // Scroll position when viewing message
  uint8_t _menu_selection;      // 0=Read, 1=Delete

  // Render functions for each view mode
  void renderListView(DisplayDriver& display);
  void renderMessageView(DisplayDriver& display);
  void renderMenuView(DisplayDriver& display);

  // Helper functions
  void formatTimestamp(uint32_t timestamp, char* buf, int buflen);
  void formatSenderInfo(const StoredMessage* msg, char* buf, int buflen);
  int getMessagePreviewLen(const char* msg);

public:
  MessagesScreen(UITask* task, MessageStore* msg_store);

  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
  void poll() override;

  // Reset to list view when screen is shown
  void reset();

  // Get current unread count
  uint8_t getUnreadCount() const;
};
