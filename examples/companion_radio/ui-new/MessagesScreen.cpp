#include "MessagesScreen.h"
#include "UITask.h"
#include "UIHelpers.h"
#include "../MyMesh.h"
#include "helpers/ui/UIStrings.h"
#include <RTClib.h>

extern MyMesh the_mesh;

// Screen layout constants
static constexpr int SCREEN_TOP_MARGIN = 20;
static constexpr int SCREEN_BOTTOM_MARGIN = 5;

// List view constants
static constexpr int LIST_LINE_HEIGHT = 20;
static constexpr int LIST_TEXT_PADDING_TOP = 1;
static constexpr int LIST_TEXT_PADDING_LEFT = 6;
static constexpr int LIST_TEXT_MAX_WIDTH_MARGIN = 12;
static constexpr int LIST_FONT_SIZE = 1;  // FreeSerif9pt7b

// Message view constants
static constexpr int MSG_VIEW_SENDER_Y = 4;
static constexpr int MSG_VIEW_TIMESTAMP_Y = 20;
static constexpr int MSG_VIEW_TEXT_START_Y = 35;
static constexpr int MSG_VIEW_TEXT_PADDING_X = 6;
static constexpr int MSG_VIEW_TEXT_MAX_WIDTH_MARGIN = 12;
static constexpr int MSG_VIEW_HEADER_FONT_SIZE = 0;
static constexpr int MSG_VIEW_TEXT_FONT_SIZE = 0;  // FreeMono9pt7b (monospace)

// Menu view constants
static constexpr int MENU_START_Y = 30;
static constexpr int MENU_SPACING = 25;
static constexpr int MENU_HEIGHT = 18;
static constexpr int MENU_MARGIN_X = 10;
static constexpr int MENU_TEXT_X = 15;
static constexpr int MENU_FONT_SIZE = 1;

// Optimized macro to draw thin dotted border around selected item
#define DRAW_SELECTION_BORDER(is_selected, y_pos, spacing) \
  if (is_selected) { \
    display.setColor(DisplayDriver::LIGHT); \
    drawDottedRect(display, 1, y_pos, display.width() - 2, (spacing) + 2, 3); \
  }

// Optimized macro to draw dotted border with custom margins (for menu items)
#define DRAW_MENU_BORDER(is_selected, y_pos, margin_x, height) \
  if (is_selected) { \
    drawDottedRect(display, margin_x, (y_pos) - 2, display.width() - 2 * (margin_x), height, 3); \
  }

MessagesScreen::MessagesScreen(UITask* task, MessageStore* msg_store, mesh::RTCClock* rtc, NodePrefs* node_prefs)
  : _task(task),
    _msg_store(msg_store),
    _rtc(rtc),
    _node_prefs(node_prefs),
    _mode(LIST_VIEW),
    _selected_idx(0),
    _scroll_offset(0),
    _message_scroll(0),
    _menu_selection(0) {
}

void MessagesScreen::reset() {
  _mode = LIST_VIEW;
  _selected_idx = 0;
  _scroll_offset = 0;
  _message_scroll = 0;
  _menu_selection = 0;
}

uint8_t MessagesScreen::getUnreadCount() const {
  return _msg_store->getUnreadCount();
}

int MessagesScreen::render(DisplayDriver& display) {
  display.clear();
  display.startFrame();

  switch (_mode) {
    case LIST_VIEW:
      renderListView(display);
      break;
    case MESSAGE_VIEW:
      renderMessageView(display);
      break;
    case MENU_VIEW:
      renderMenuView(display);
      break;
  }

  return 1000; // Refresh every second
}

void MessagesScreen::renderListView(DisplayDriver& display) {
  uint8_t total = _msg_store->getCount();
  uint8_t unread = _msg_store->getUnreadCount();

  // Draw header
  char header[32];
  if (unread > 0) {
    snprintf(header, sizeof(header), "%s %d/%d", TR(STR_MESSAGES), unread, total);
  } else {
    snprintf(header, sizeof(header), "%s %d", TR(STR_MESSAGES), total);
  }
  DRAW_SCREEN_HEADER(header, _task);

  if (total == 0) {
    // No messages
    display.setTextSize(1);
    display.setCursor(10, 30);
    display.print(TR(STR_NO_SAVED_MESSAGES));
    return;
  }

  // Calculate visible area (cache these calculations)
  int y = SCREEN_TOP_MARGIN;
  if (cached_screen_height == 0) {
    cached_screen_height = display.height();
    cached_available_height = cached_screen_height - SCREEN_TOP_MARGIN - SCREEN_BOTTOM_MARGIN;
    cached_visible_items = cached_available_height / LIST_LINE_HEIGHT;
    cached_max_text_width = display.width() - LIST_TEXT_MAX_WIDTH_MARGIN;
  }

  // Adjust scroll offset to keep selected item visible
  if (_selected_idx < _scroll_offset) {
    _scroll_offset = _selected_idx;
  } else if (_selected_idx >= _scroll_offset + cached_visible_items) {
    _scroll_offset = _selected_idx - cached_visible_items + 1;
  }

  // Draw messages
  for (uint8_t i = _scroll_offset; i < total && i < _scroll_offset + cached_visible_items; i++) {
    const StoredMessage* msg = _msg_store->getMessage(i);
    if (!msg) continue;

    bool is_selected = (i == _selected_idx);
    bool is_unread = !(msg->flags & 0x01);

    display.setColor(DisplayDriver::LIGHT);

    // Draw thick border around selected item (height includes padding)
    DRAW_SELECTION_BORDER(is_selected, y, LIST_LINE_HEIGHT);

    // Draw sender and preview on same line with top padding
    display.setTextSize(LIST_FONT_SIZE);

    // Build combined string: "sender: message preview..."
    char combined[64];
    char unread_marker = is_unread ? '*' : ' ';

    // Start with sender name
    int offset = snprintf(combined, sizeof(combined), "%c%s: ", unread_marker, msg->sender_name);

    // Add as much of the message as will fit
    int remaining = sizeof(combined) - offset - 4;  // Reserve space for "..."
    if (remaining > 0) {
      strncpy(combined + offset, msg->msg, remaining);
      combined[offset + remaining] = '\0';
    }

    // Use ellipsized drawing to handle overflow with top padding inside border (use cached width)
    display.drawTextEllipsized(LIST_TEXT_PADDING_LEFT, y + LIST_TEXT_PADDING_TOP, cached_max_text_width, combined);

    y += LIST_LINE_HEIGHT;
  }

  // Draw scroll indicator if needed (use cached values)
  if (total > cached_visible_items) {
    display.setColor(DisplayDriver::LIGHT);
    int indicator_height = (cached_visible_items * cached_available_height) / total;
    int indicator_y = SCREEN_TOP_MARGIN + (_scroll_offset * cached_available_height) / total;
    display.drawRect(display.width() - 3, indicator_y, 2, indicator_height);
  }

  // Footer removed for cleaner UI
}

void MessagesScreen::renderMessageView(DisplayDriver& display) {
  const StoredMessage* msg = _msg_store->getMessage(_selected_idx);
  if (!msg) {
    _mode = LIST_VIEW;
    return;
  }

  // Draw header with sender info (with top margin)
  display.setTextSize(MSG_VIEW_HEADER_FONT_SIZE);
  display.setColor(DisplayDriver::YELLOW);
  display.setCursor(MSG_VIEW_TEXT_PADDING_X / 3, MSG_VIEW_SENDER_Y);

  char sender_info[64];
  formatSenderInfo(msg, sender_info, sizeof(sender_info));
  display.print(sender_info);

  // Draw timestamp on second line with better spacing
  display.setColor(DisplayDriver::LIGHT);
  display.setCursor(MSG_VIEW_TEXT_PADDING_X / 3, MSG_VIEW_TIMESTAMP_Y);
  char time_buf[32];
  formatTimestamp(msg->timestamp, time_buf, sizeof(time_buf));
  display.print(time_buf);

  // Draw message text with better formatting
  int screen_height = display.height();
  int available_height = screen_height - MSG_VIEW_TEXT_START_Y - SCREEN_BOTTOM_MARGIN;

  // Use monospace font for message text
  display.setTextSize(MSG_VIEW_TEXT_FONT_SIZE);
  display.setColor(DisplayDriver::LIGHT);
  display.setCursor(MSG_VIEW_TEXT_PADDING_X, MSG_VIEW_TEXT_START_Y);

  // Word wrap the message with proper margins
  display.printWordWrap(msg->msg, display.width() - MSG_VIEW_TEXT_MAX_WIDTH_MARGIN);

  // Footer removed for cleaner UI
}

void MessagesScreen::renderMenuView(DisplayDriver& display) {
  const StoredMessage* msg = _msg_store->getMessage(_selected_idx);
  if (!msg) {
    _mode = LIST_VIEW;
    return;
  }

  // Draw header
  DRAW_SCREEN_HEADER(TR(STR_OPTIONS), _task);

  // Draw menu options
  int y = MENU_START_Y;
  display.setTextSize(MENU_FONT_SIZE);
  display.setColor(DisplayDriver::LIGHT);

  // Option 0: Back to Read
  DRAW_MENU_BORDER(_menu_selection == 0, y, MENU_MARGIN_X, MENU_HEIGHT);
  display.setCursor(MENU_TEXT_X, y);
  display.print(TR(STR_READ));

  y += MENU_SPACING;

  // Option 1: Delete
  DRAW_MENU_BORDER(_menu_selection == 1, y, MENU_MARGIN_X, MENU_HEIGHT);
  display.setCursor(MENU_TEXT_X, y);
  display.print(TR(STR_DELETE));
}

bool MessagesScreen::handleInput(char c) {
  switch (_mode) {
    case LIST_VIEW: {
      // Always handle cancel button, even with no messages
      if (c == KEY_CANCEL) {
        _task->gotoHomeScreen();
        return true;
      }

      uint8_t total = _msg_store->getCount();
      if (total == 0) {
        return false; // No messages, no other input to handle
      }

      if (c == KEY_UP || c == KEY_PREV) {
        if (_selected_idx > 0) {
          _selected_idx--;
        } else {
          _selected_idx = total - 1; // Wrap to bottom
        }
        return true;
      }

      if (c == KEY_DOWN || c == KEY_NEXT) {
        _selected_idx = (_selected_idx + 1) % total;
        return true;
      }

      if (c == KEY_ENTER || c == KEY_SELECT) {
        // Show options menu
        _menu_selection = 0;
        _mode = MENU_VIEW;
        return true;
      }

      if (c == KEY_RIGHT) {
        // Open selected message directly with right arrow
        _msg_store->markAsRead(_selected_idx);
        _message_scroll = 0;
        _mode = MESSAGE_VIEW;
        return true;
      }

      if (c == KEY_CONTEXT_MENU) {
        // Show delete menu (same as ENTER/SELECT)
        _menu_selection = 0;
        _mode = MENU_VIEW;
        return true;
      }

      break;
    }

    case MESSAGE_VIEW: {
      if (c == KEY_UP || c == KEY_PREV) {
        // Scroll up in message
        if (_message_scroll > 0) {
          _message_scroll--;
        }
        return true;
      }

      if (c == KEY_DOWN || c == KEY_NEXT) {
        // Scroll down in message
        _message_scroll++;
        return true;
      }

      if (c == KEY_CONTEXT_MENU) {
        // Show options menu
        _menu_selection = 0;
        _mode = MENU_VIEW;
        return true;
      }

      if (c == KEY_CANCEL) {
        // Back to list
        _mode = LIST_VIEW;
        return true;
      }

      break;
    }

    case MENU_VIEW: {
      if (c == KEY_UP || c == KEY_PREV) {
        _menu_selection = (_menu_selection == 0) ? 1 : 0;
        return true;
      }

      if (c == KEY_DOWN || c == KEY_NEXT) {
        _menu_selection = (_menu_selection + 1) % 2;
        return true;
      }

      if (c == KEY_ENTER || c == KEY_SELECT) {
        if (_menu_selection == 0) {
          // Back to read
          _mode = MESSAGE_VIEW;
        } else if (_menu_selection == 1) {
          // Delete message
          _msg_store->deleteMessage(_selected_idx);

          // Adjust selection if needed
          uint8_t total = _msg_store->getCount();
          if (_selected_idx >= total && total > 0) {
            _selected_idx = total - 1;
          }

          // Back to list
          _mode = LIST_VIEW;
        }
        return true;
      }

      if (c == KEY_CANCEL) {
        // Cancel menu
        _mode = MESSAGE_VIEW;
        return true;
      }

      break;
    }
  }

  return false;
}

void MessagesScreen::poll() {
  // Check if we need to save messages periodically
  if (_msg_store->isDirty()) {
    static unsigned long last_save = 0;
    if (millis() - last_save > 5000) { // Save every 5 seconds if dirty
      _msg_store->save();
      last_save = millis();
    }
  }
}

void MessagesScreen::formatTimestamp(uint32_t timestamp, char* buf, int buflen) {
  // Convert Unix timestamp to actual date/time
  DateTime dt = DateTime(timestamp);

  // Format as: HH:MM - DD/MM/YY
  snprintf(buf, buflen, "%02d:%02d - %d/%d/%d",
           dt.hour(), dt.minute(),
           dt.day(), dt.month(), dt.year() % 100);
}

void MessagesScreen::formatSenderInfo(const StoredMessage* msg, char* buf, int buflen) {
  if (msg->path_len == 0xFF) {
    snprintf(buf, buflen, "%s %s", TR(STR_FROM), msg->sender_name);
  } else {
    snprintf(buf, buflen, "%s %s (h%d)", TR(STR_FROM), msg->sender_name, msg->path_len);
  }
}

int MessagesScreen::getMessagePreviewLen(const char* msg) {
  // Get up to 20 characters for preview, breaking at space if possible
  int len = strlen(msg);
  if (len <= 20) return len;

  // Try to break at a space
  for (int i = 19; i >= 15; i--) {
    if (msg[i] == ' ') {
      return i;
    }
  }

  return 20; // Just truncate
}
