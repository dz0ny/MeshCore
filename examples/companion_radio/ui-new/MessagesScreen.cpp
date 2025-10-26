#include "MessagesScreen.h"
#include "UITask.h"
#include "UIHelpers.h"
#include "../MyMesh.h"

extern MyMesh the_mesh;

static constexpr int SCREEN_TOP_MARGIN = 20;
static constexpr int SCREEN_BOTTOM_MARGIN = 5;

// Macro to draw thin dotted border around selected item
#define DRAW_SELECTION_BORDER(is_selected, y_pos, spacing) \
  if (is_selected) { \
    display.setColor(DisplayDriver::LIGHT); \
    int border_x = 1, border_y = (y_pos), border_w = display.width() - 2, border_h = (spacing) + 2; \
    for (int x = border_x; x < border_x + border_w; x += 3) { \
      display.fillRect(x, border_y, 1, 1); \
    } \
    for (int x = border_x; x < border_x + border_w; x += 3) { \
      display.fillRect(x, border_y + border_h - 1, 1, 1); \
    } \
    for (int y = border_y; y < border_y + border_h; y += 3) { \
      display.fillRect(border_x, y, 1, 1); \
    } \
    for (int y = border_y; y < border_y + border_h; y += 3) { \
      display.fillRect(border_x + border_w - 1, y, 1, 1); \
    } \
    display.setColor(DisplayDriver::LIGHT); \
  }

// Macro to draw dotted border with custom margins (for menu items)
#define DRAW_MENU_BORDER(is_selected, y_pos, margin_x, height) \
  if (is_selected) { \
    int bx = (margin_x), by = (y_pos) - 2, bw = display.width() - 2 * (margin_x), bh = (height); \
    for (int x = bx; x < bx + bw; x += 3) display.fillRect(x, by, 1, 1); \
    for (int x = bx; x < bx + bw; x += 3) display.fillRect(x, by + bh - 1, 1, 1); \
    for (int y = by; y < by + bh; y += 3) display.fillRect(bx, y, 1, 1); \
    for (int y = by; y < by + bh; y += 3) display.fillRect(bx + bw - 1, y, 1, 1); \
  }

MessagesScreen::MessagesScreen(UITask* task, MessageStore* msg_store)
  : _task(task),
    _msg_store(msg_store),
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
    snprintf(header, sizeof(header), "Messages %d/%d", unread, total);
  } else {
    snprintf(header, sizeof(header), "Messages %d", total);
  }
  DRAW_SCREEN_HEADER(header, _task);

  if (total == 0) {
    // No messages
    display.setTextSize(1);
    display.setCursor(10, 30);
    display.print("No saved messages");
    return;
  }

  // Calculate visible area
  int y = SCREEN_TOP_MARGIN;
  int screen_height = display.height();
  int available_height = screen_height - SCREEN_TOP_MARGIN - SCREEN_BOTTOM_MARGIN;
  int line_height = 18; // Height per message entry
  int visible_items = available_height / line_height;

  // Adjust scroll offset to keep selected item visible
  if (_selected_idx < _scroll_offset) {
    _scroll_offset = _selected_idx;
  } else if (_selected_idx >= _scroll_offset + visible_items) {
    _scroll_offset = _selected_idx - visible_items + 1;
  }

  // Draw messages
  for (uint8_t i = _scroll_offset; i < total && i < _scroll_offset + visible_items; i++) {
    const StoredMessage* msg = _msg_store->getMessage(i);
    if (!msg) continue;

    bool is_selected = (i == _selected_idx);
    bool is_unread = !(msg->flags & 0x01);

    display.setColor(DisplayDriver::LIGHT);

    // Draw thick border around selected item
    DRAW_SELECTION_BORDER(is_selected, y, line_height);

    // Draw sender name
    display.setTextSize(0);
    display.setCursor(6, y);

    char sender_buf[MAX_SENDER_NAME_LEN + 10];
    if (is_unread) {
      snprintf(sender_buf, sizeof(sender_buf), "*%s", msg->sender_name);
    } else {
      snprintf(sender_buf, sizeof(sender_buf), " %s", msg->sender_name);
    }
    display.print(sender_buf);

    // Draw message preview on second line
    display.setCursor(6, y + 9);
    char preview[23];
    int preview_len = getMessagePreviewLen(msg->msg);
    strncpy(preview, msg->msg, preview_len);
    preview[preview_len] = '\0';
    if (msg->msg_len > preview_len) {
      strcat(preview, "...");
    }
    display.print(preview);

    y += line_height;
  }

  // Draw scroll indicator if needed
  if (total > visible_items) {
    display.setColor(DisplayDriver::LIGHT);
    int indicator_height = (visible_items * available_height) / total;
    int indicator_y = SCREEN_TOP_MARGIN + (_scroll_offset * available_height) / total;
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

  // Draw header with sender info
  display.setTextSize(0);
  display.setColor(DisplayDriver::YELLOW);
  display.setCursor(0, 2);

  char sender_info[64];
  formatSenderInfo(msg, sender_info, sizeof(sender_info));
  display.print(sender_info);

  // Draw timestamp on second line with better spacing
  display.setColor(DisplayDriver::LIGHT);
  display.setCursor(0, 11);
  char time_buf[32];
  formatTimestamp(msg->timestamp, time_buf, sizeof(time_buf));
  display.print(time_buf);

  // Draw separator line with better spacing
  display.setColor(DisplayDriver::LIGHT);
  for (int dx = 0; dx < display.width(); dx += 3) {
    display.fillRect(dx, 22, 1, 1);
  }

  // Draw message text with better formatting
  int y = 26; // Start below separator with margin
  int screen_height = display.height();
  int available_height = screen_height - y - SCREEN_BOTTOM_MARGIN;

  // Use smaller text size for better readability
  display.setTextSize(0);
  display.setColor(DisplayDriver::LIGHT);
  display.setCursor(4, y);

  // Word wrap the message with proper margins
  display.printWordWrap(msg->msg, display.width() - 8);

  // Footer removed for cleaner UI
}

void MessagesScreen::renderMenuView(DisplayDriver& display) {
  const StoredMessage* msg = _msg_store->getMessage(_selected_idx);
  if (!msg) {
    _mode = LIST_VIEW;
    return;
  }

  // Draw header
  DRAW_SCREEN_HEADER("Options", _task);

  // Draw menu options
  int y = 30;
  int menu_spacing = 25;
  int menu_height = 18;
  display.setTextSize(1);
  display.setColor(DisplayDriver::LIGHT);

  // Option 0: Back to Read
  DRAW_MENU_BORDER(_menu_selection == 0, y, 10, menu_height);
  display.setCursor(15, y);
  display.print("Read");

  y += menu_spacing;

  // Option 1: Delete
  DRAW_MENU_BORDER(_menu_selection == 1, y, 10, menu_height);
  display.setCursor(15, y);
  display.print("Delete");
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

      if (c == KEY_ENTER || c == KEY_SELECT || c == KEY_RIGHT) {
        // Open selected message
        _msg_store->markAsRead(_selected_idx);
        _message_scroll = 0;
        _mode = MESSAGE_VIEW;
        return true;
      }

      if (c == KEY_CONTEXT_MENU) {
        // Show delete menu
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
  // Simple timestamp formatting (adjust based on your RTC implementation)
  unsigned long seconds = timestamp;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;
  unsigned long days = hours / 24;

  if (days > 0) {
    snprintf(buf, buflen, "%lud %luh ago", days, hours % 24);
  } else if (hours > 0) {
    snprintf(buf, buflen, "%luh %lum ago", hours, minutes % 60);
  } else if (minutes > 0) {
    snprintf(buf, buflen, "%lum ago", minutes);
  } else {
    snprintf(buf, buflen, "Just now");
  }
}

void MessagesScreen::formatSenderInfo(const StoredMessage* msg, char* buf, int buflen) {
  if (msg->path_len == 0xFF) {
    snprintf(buf, buflen, "From: %s", msg->sender_name);
  } else {
    snprintf(buf, buflen, "From: %s (h%d)", msg->sender_name, msg->path_len);
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
