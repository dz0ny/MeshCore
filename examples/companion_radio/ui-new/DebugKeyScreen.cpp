#include "DebugKeyScreen.h"
#include "UITask.h"
#include <Arduino.h>

static constexpr int SCREEN_TOP_MARGIN = 22;

DebugKeyScreen::DebugKeyScreen(UITask* task)
  : _task(task), _history_count(0), _scroll_offset(0) {
  memset(_key_history, 0, sizeof(_key_history));
}

const char* DebugKeyScreen::getKeyName(char key) {
  switch (key) {
    case KEY_LEFT: return "LEFT";
    case KEY_UP: return "UP";
    case KEY_DOWN: return "DOWN";
    case KEY_RIGHT: return "RIGHT";
    case KEY_SELECT: return "SELECT";
    case KEY_ENTER: return "ENTER";
    case KEY_CANCEL: return "CANCEL";
    case KEY_HOME: return "HOME";
    case KEY_NEXT: return "NEXT";
    case KEY_PREV: return "PREV";
    case KEY_CONTEXT_MENU: return "MENU";
    default:
      if (key >= 32 && key <= 126) {
        static char buf[2];
        buf[0] = key;
        buf[1] = '\0';
        return buf;
      }
      return "???";
  }
}

int DebugKeyScreen::render(DisplayDriver& display) {
  display.clear();
  display.startFrame();

  // Draw header
  display.setTextSize(0);
  display.setColor(DisplayDriver::RED);
  display.setCursor(0, 0);
  display.print("DEBUG: Key Presses");

  // Draw warning
  display.setTextSize(0);
  display.setColor(DisplayDriver::YELLOW);
  display.setCursor(0, 8);
  display.print("RESTART DEVICE TO EXIT");

  // Draw separator
  display.setColor(DisplayDriver::LIGHT);
  for (int dx = 0; dx < display.width(); dx += 3) {
    display.fillRect(dx, 18, 1, 1);
  }

  // Draw key history
  display.setTextSize(0);
  display.setColor(DisplayDriver::LIGHT);

  int y = SCREEN_TOP_MARGIN;
  int line_height = display.getTextHeight("A") + 2;
  int available_height = display.height() - SCREEN_TOP_MARGIN - 10;
  int visible_lines = available_height / line_height;

  if (_history_count == 0) {
    display.setCursor(5, y + 10);
    display.print("No keys pressed yet...");
  } else {
    // Show most recent keys first
    int start_idx = max(0, _history_count - visible_lines - _scroll_offset);
    int end_idx = min(_history_count, start_idx + visible_lines);

    for (int i = end_idx - 1; i >= start_idx; i--) {
      char buf[40];
      unsigned long elapsed = (millis() - _key_history[i].timestamp) / 1000;
      snprintf(buf, sizeof(buf), "%2d: %10s (0x%02X) %lus ago",
               _history_count - i,
               getKeyName(_key_history[i].key),
               (uint8_t)_key_history[i].key,
               elapsed);

      display.setCursor(2, y);
      display.print(buf);
      y += line_height;
    }
  }

  // Draw count at bottom
  display.setColor(DisplayDriver::GREEN);
  char count_buf[30];
  snprintf(count_buf, sizeof(count_buf), "Total: %d keys", _history_count);
  display.setCursor(2, display.height() - 10);
  display.print(count_buf);

  return 500; // Refresh every 500ms to update timestamps
}

bool DebugKeyScreen::handleInput(char c) {
  // Log ALL key presses - no exits allowed!
  // This is a debug screen to see what buttons generate what codes
  if (_history_count < MAX_KEY_HISTORY) {
    _key_history[_history_count].key = c;
    _key_history[_history_count].timestamp = millis();
    _history_count++;
  } else {
    // Shift history and add new key
    for (int i = 0; i < MAX_KEY_HISTORY - 1; i++) {
      _key_history[i] = _key_history[i + 1];
    }
    _key_history[MAX_KEY_HISTORY - 1].key = c;
    _key_history[MAX_KEY_HISTORY - 1].timestamp = millis();
  }

  // Allow scrolling with up/down
  if (c == KEY_UP || c == KEY_PREV) {
    if (_scroll_offset > 0) {
      _scroll_offset--;
    }
  } else if (c == KEY_DOWN || c == KEY_NEXT) {
    _scroll_offset++;
  }

  // Consume all keys
  return true;
}
