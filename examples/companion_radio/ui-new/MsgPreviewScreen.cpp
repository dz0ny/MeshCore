#include "MsgPreviewScreen.h"
#include "UITask.h"
#include "UIHelpers.h"
#include "helpers/ui/UIStrings.h"
#include <helpers/TxtDataHelpers.h>
#include <MeshCore.h>

#ifndef AUTO_OFF_MILLIS
  #define AUTO_OFF_MILLIS     15000   // 15 seconds
#endif

MsgPreviewScreen::MsgPreviewScreen(UITask* task, mesh::RTCClock* rtc)
  : _task(task), _rtc(rtc), _mode(PREVIEW_VIEW), _menu_selection(0) {
  num_unread = 0;
}

void MsgPreviewScreen::addPreview(uint8_t path_len, const char* from_name, const char* msg) {
  if (num_unread >= MAX_UNREAD_MSGS) return;  // full

  auto p = &unread[num_unread++];
  p->timestamp = _rtc->getCurrentTime();
  if (path_len == 0xFF) {
    sprintf(p->origin, "(D) %s:", from_name);
  } else {
    sprintf(p->origin, "(%d) %s:", (uint32_t) path_len, from_name);
  }
  StrHelper::strncpy(p->msg, msg, sizeof(p->msg));
}

int MsgPreviewScreen::render(DisplayDriver& display) {
  switch (_mode) {
    case PREVIEW_VIEW:
      renderPreviewView(display);
      break;
    case MENU_VIEW:
      renderMenuView(display);
      break;
  }

#if AUTO_OFF_MILLIS==0 // probably e-ink
  return 10000; // 10 s
#else
  return 1000;  // next render after 1000 ms
#endif
}

void MsgPreviewScreen::renderPreviewView(DisplayDriver& display) {
  char tmp[16];
  display.setTextSize(1);
  int line_spacing = display.getTextHeight("A") + 2;  // font height + small gap
  int y = 0;

  display.setCursor(0, y);
  display.setColor(DisplayDriver::GREEN);
  sprintf(tmp, "Unread: %d", num_unread);
  display.print(tmp);

  auto p = &unread[0];

  int secs = _rtc->getCurrentTime() - p->timestamp;
  if (secs < 60) {
    sprintf(tmp, "%ds", secs);
  } else if (secs < 60*60) {
    sprintf(tmp, "%dm", secs / 60);
  } else {
    sprintf(tmp, "%dh", secs / (60*60));
  }
  display.setCursor(display.width() - display.getTextWidth(tmp) - 2, y);
  display.print(tmp);

  y = y + line_spacing;
  display.drawRect(0, y, display.width(), 1);  // horiz line

  y = y + 3;  // small gap after line
  display.setCursor(0, y);
  display.setColor(DisplayDriver::YELLOW);
  char filtered_origin[sizeof(p->origin)];
  display.translateUTF8ToBlocks(filtered_origin, p->origin, sizeof(filtered_origin));
  display.print(filtered_origin);

  y = y + line_spacing;
  display.setCursor(0, y);
  display.setColor(DisplayDriver::LIGHT);
  char filtered_msg[sizeof(p->msg)];
  display.translateUTF8ToBlocks(filtered_msg, p->msg, sizeof(filtered_msg));
  display.printWordWrap(filtered_msg, display.width());
}

void MsgPreviewScreen::renderMenuView(DisplayDriver& display) {
  // Menu constants
  static constexpr int MENU_START_Y = 30;
  static constexpr int MENU_SPACING = 25;
  static constexpr int MENU_HEIGHT = 18;
  static constexpr int MENU_MARGIN_X = 10;
  static constexpr int MENU_TEXT_X = 15;

  // Draw header
  display.setTextSize(1);
  display.setColor(DisplayDriver::LIGHT);
  display.setCursor(0, 0);
  display.print("Options");

  // Draw menu options
  int y = MENU_START_Y;
  display.setTextSize(1);
  display.setColor(DisplayDriver::LIGHT);

  // Option 0: Skip (next message)
  if (_menu_selection == 0) {
    drawDottedRect(display, MENU_MARGIN_X, y - 2, display.width() - 2 * MENU_MARGIN_X, MENU_HEIGHT, 3);
  }
  display.setCursor(MENU_TEXT_X, y);
  display.print("Skip");

  y += MENU_SPACING;

  // Option 1: Delete (current message)
  if (_menu_selection == 1) {
    drawDottedRect(display, MENU_MARGIN_X, y - 2, display.width() - 2 * MENU_MARGIN_X, MENU_HEIGHT, 3);
  }
  display.setCursor(MENU_TEXT_X, y);
  display.print("Delete");

  y += MENU_SPACING;

  // Option 2: Clear All
  if (_menu_selection == 2) {
    drawDottedRect(display, MENU_MARGIN_X, y - 2, display.width() - 2 * MENU_MARGIN_X, MENU_HEIGHT, 3);
  }
  display.setCursor(MENU_TEXT_X, y);
  display.print("Clear All");
}

bool MsgPreviewScreen::handleInput(char c) {
  switch (_mode) {
    case PREVIEW_VIEW: {
      // Show menu on select/enter
      if (c == KEY_SELECT || c == KEY_ENTER) {
        _menu_selection = 0;
        _mode = MENU_VIEW;
        return true;
      }

      // Skip to next message
      if (c == KEY_NEXT || c == KEY_RIGHT) {
        num_unread--;
        if (num_unread == 0) {
          _task->gotoHomeScreen();
        } else {
          // delete first/curr item from unread queue
          for (int i = 0; i < num_unread; i++) {
            unread[i] = unread[i + 1];
          }
        }
        return true;
      }

      // Cancel returns to home
      if (c == KEY_CANCEL) {
        _task->gotoHomeScreen();
        return true;
      }

      break;
    }

    case MENU_VIEW: {
      // Navigate menu
      if (c == KEY_UP || c == KEY_PREV) {
        if (_menu_selection > 0) {
          _menu_selection--;
        } else {
          _menu_selection = 2; // Wrap to bottom (Clear All)
        }
        return true;
      }

      if (c == KEY_DOWN || c == KEY_NEXT) {
        _menu_selection = (_menu_selection + 1) % 3; // 3 menu options
        return true;
      }

      // Execute selected action
      if (c == KEY_ENTER || c == KEY_SELECT) {
        if (_menu_selection == 0) {
          // Skip - go to next message
          num_unread--;
          if (num_unread == 0) {
            _task->gotoHomeScreen();
          } else {
            // delete first/curr item from unread queue
            for (int i = 0; i < num_unread; i++) {
              unread[i] = unread[i + 1];
            }
            _mode = PREVIEW_VIEW; // Back to preview
          }
        } else if (_menu_selection == 1) {
          // Delete current message
          num_unread--;
          if (num_unread == 0) {
            _task->gotoHomeScreen();
          } else {
            // delete first/curr item from unread queue
            for (int i = 0; i < num_unread; i++) {
              unread[i] = unread[i + 1];
            }
            _mode = PREVIEW_VIEW; // Back to preview
          }
        } else if (_menu_selection == 2) {
          // Clear all
          num_unread = 0;
          _task->gotoHomeScreen();
        }
        return true;
      }

      // Cancel menu - back to preview
      if (c == KEY_CANCEL) {
        _mode = PREVIEW_VIEW;
        return true;
      }

      break;
    }
  }

  return false;
}
