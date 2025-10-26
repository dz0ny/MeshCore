#include "ReportsScreen.h"
#include "UITask.h"
#include "UIHelpers.h"

static constexpr int SCREEN_TOP_MARGIN = 20;
static constexpr int SCREEN_BOTTOM_MARGIN = 5;

// Macro to draw thin dotted border around selected item
#define DRAW_SELECTION_BORDER(is_selected, y_pos, spacing) \
  if (is_selected) { \
    display.setColor(DisplayDriver::LIGHT); \
    int border_x = 1, border_y = (y_pos), border_w = display.width() - 2, border_h = (spacing) + 2; \
    /* Top border (dotted) */ \
    for (int x = border_x; x < border_x + border_w; x += 3) { \
      display.fillRect(x, border_y, 1, 1); \
    } \
    /* Bottom border (dotted) */ \
    for (int x = border_x; x < border_x + border_w; x += 3) { \
      display.fillRect(x, border_y + border_h - 1, 1, 1); \
    } \
    /* Left border (dotted) */ \
    for (int y = border_y; y < border_y + border_h; y += 3) { \
      display.fillRect(border_x, y, 1, 1); \
    } \
    /* Right border (dotted) */ \
    for (int y = border_y; y < border_y + border_h; y += 3) { \
      display.fillRect(border_x + border_w - 1, y, 1, 1); \
    } \
    display.setColor(DisplayDriver::GREEN); \
  }

ReportsScreen::ReportsScreen(UITask* task)
  : _task(task), _selected_item(0) {
}

void ReportsScreen::reset() {
  _selected_item = 0;
}

int ReportsScreen::render(DisplayDriver& display) {
  display.clear();
  display.startFrame();

  // Draw header
  DRAW_SCREEN_HEADER("Reports", _task);

  // Display menu items
  display.setColor(DisplayDriver::GREEN);
  display.setTextSize(1);
  int line_height = display.getTextHeight("A");
  int line_spacing = line_height + 6;

  int num_items = 4;  // GPS Info, Radio Stats, Telemetry, Debug
  int available_height = display.height() - SCREEN_TOP_MARGIN - SCREEN_BOTTOM_MARGIN;
  int total_height = num_items * line_spacing;
  int y = SCREEN_TOP_MARGIN + ((available_height - total_height) / 2) + (line_height / 2);

  char buf[50];

  // GPS Info
  DRAW_SELECTION_BORDER(_selected_item == 0, y, line_spacing);
  sprintf(buf, "  Show GPS Info");
  display.drawTextLeftAlign(5, y, buf);
  y += line_spacing;

  // Radio Stats
  DRAW_SELECTION_BORDER(_selected_item == 1, y, line_spacing);
  sprintf(buf, "  Show Radio Stats");
  display.drawTextLeftAlign(5, y, buf);
  y += line_spacing;

  // Telemetry
  DRAW_SELECTION_BORDER(_selected_item == 2, y, line_spacing);
  sprintf(buf, "  Show Telemetry");
  display.drawTextLeftAlign(5, y, buf);
  y += line_spacing;

  // Debug
  DRAW_SELECTION_BORDER(_selected_item == 3, y, line_spacing);
  sprintf(buf, "  Show Debug Keys");
  display.drawTextLeftAlign(5, y, buf);

  return 1000; // Refresh every second
}

bool ReportsScreen::handleInput(char c) {
  if (c == KEY_UP || c == KEY_PREV) {
    if (_selected_item > 0) {
      _selected_item--;
    } else {
      _selected_item = 3;  // Wrap to bottom
    }
    return true;
  }

  if (c == KEY_DOWN || c == KEY_NEXT) {
    _selected_item = (_selected_item + 1) % 4;
    return true;
  }

  if (c == KEY_ENTER || c == KEY_SELECT) {
    // Navigate to the selected screen
    switch (_selected_item) {
      case 0:  // GPS Info
        _task->gotoGPSScreen();
        break;
      case 1:  // Radio Stats
        _task->gotoRadioStatsScreen();
        break;
      case 2:  // Telemetry
        _task->gotoTelemetryScreen();
        break;
      case 3:  // Debug
        _task->gotoDebugKeyScreen();
        break;
    }
    return true;
  }

  if (c == KEY_CANCEL || c == KEY_LEFT) {
    // Back to home
    _task->gotoHomeScreen();
    return true;
  }

  return false;
}

void ReportsScreen::poll() {
  // Nothing to poll
}
