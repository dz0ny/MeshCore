#include "ContactsScreen.h"
#include "UITask.h"
#include "UIHelpers.h"
#include "../MyMesh.h"
#include "helpers/ui/UIStrings.h"

extern MyMesh the_mesh;

static constexpr int SCREEN_TOP_MARGIN = 20;
static constexpr int SCREEN_BOTTOM_MARGIN = 5;
static constexpr int MAX_LIST_ITEMS = 2;        // Maximum items to show at once
static constexpr int ITEM_HEIGHT_NO_GPS = 12;   // Height for items without GPS
static constexpr int ITEM_HEIGHT_WITH_GPS = 34; // Height for items with GPS (name + GPS line) - 10px extra padding

// Optimized macro to draw thin dotted border around selected item
#define DRAW_SELECTION_BORDER(is_selected, y_pos, spacing) \
  if (is_selected) { \
    display.setColor(DisplayDriver::LIGHT); \
    drawDottedRect(display, 1, y_pos, display.width() - 2, (spacing) + 2, 3); \
  }

// Macro to render a single contact list item with dynamic height based on content
#define RENDER_CONTACT_ITEM(advert, y_pos, is_selected, rtc_time, height_var) \
  do { \
    bool has_gps = ((advert)->gps_lat != 0 || (advert)->gps_lon != 0); \
    height_var = has_gps ? ITEM_HEIGHT_WITH_GPS : ITEM_HEIGHT_NO_GPS; \
    DRAW_SELECTION_BORDER(is_selected, y_pos, height_var); \
    display.setColor(DisplayDriver::LIGHT); \
    display.setTextSize(1); \
    char tmp[16]; \
    int secs = rtc_time - (advert)->recv_timestamp; \
    if (secs < 60) { \
      sprintf(tmp, "%ds", secs); \
    } else if (secs < 60*60) { \
      sprintf(tmp, "%dm", secs / 60); \
    } else { \
      sprintf(tmp, "%dh", secs / (60*60)); \
    } \
    int timestamp_width = display.getTextWidth(tmp); \
    int max_name_width = display.width() - timestamp_width - 8; \
    char filtered_name[sizeof((advert)->name)]; \
    display.translateUTF8ToBlocks(filtered_name, (advert)->name, sizeof(filtered_name)); \
    display.drawTextEllipsized(6, y_pos + 2, max_name_width, filtered_name); \
    display.setCursor(display.width() - timestamp_width - 2, y_pos + 2); \
    display.print(tmp); \
    if (has_gps) { \
      display.setTextSize(0); \
      char gps_buf[32]; \
      sprintf(gps_buf, "%.5f,%.5f", (advert)->gps_lat / 1000000.0, (advert)->gps_lon / 1000000.0); \
      display.setCursor(10, y_pos + 14); \
      display.print(gps_buf); \
      display.setTextSize(1); \
    } \
  } while(0)

// Macro to render list with max 2 items, accounting for variable item heights
#define RENDER_CONTACT_LIST(adverts, count, selected, scroll_offset, y_start, rtc_time) \
  do { \
    int items_to_show = (count - scroll_offset) > MAX_LIST_ITEMS ? MAX_LIST_ITEMS : (count - scroll_offset); \
    int list_y = y_start; \
    int rendered = 0; \
    for (int idx = scroll_offset; idx < count && rendered < items_to_show; idx++) { \
      bool is_selected = (idx == selected); \
      int item_height; \
      RENDER_CONTACT_ITEM(adverts[idx], list_y, is_selected, rtc_time, item_height); \
      list_y += item_height + 2; \
      rendered++; \
    } \
  } while(0)

ContactsScreen::ContactsScreen(UITask* task, mesh::RTCClock* rtc, SensorManager* sensors, NodePrefs* node_prefs)
  : _task(task), _rtc(rtc), _sensors(sensors), _node_prefs(node_prefs), _selected_index(0) {
}

void ContactsScreen::reset() {
  _selected_index = 0;
}

int ContactsScreen::render(DisplayDriver& display) {
  display.clear();
  display.startFrame();

  // Get recently heard nodes
  the_mesh.getRecentlyHeard(recent, UI_RECENT_LIST_SIZE);

  // Compact valid contacts into an array
  AdvertPath* contacts[UI_RECENT_LIST_SIZE];
  int contact_count = 0;
  for (int i = 0; i < UI_RECENT_LIST_SIZE; i++) {
    if (recent[i].name[0] != 0) {
      contacts[contact_count++] = &recent[i];
    }
  }

  // Draw header
  DRAW_SCREEN_HEADER(TR(STR_RECENT_CONTACTS), _task);

  if (contact_count == 0) {
    // No contacts
    display.setColor(DisplayDriver::GREEN);
    display.setTextSize(1);
    int y = (display.height() / 2) - 8;
    display.drawTextCentered(display.width() / 2, y, TR(STR_NO_NEARBY_CONTACTS));
    display.endFrame();
    return 1000;
  }

  // Ensure selection is valid
  if (_selected_index >= contact_count) {
    _selected_index = contact_count - 1;
  }
  if (_selected_index < 0) {
    _selected_index = 0;
  }

  // Calculate scroll offset to keep selected item visible (2-item window)
  int scroll_offset = 0;
  if (contact_count > MAX_LIST_ITEMS) {
    // Keep selected item in view within 2-item window
    if (_selected_index >= MAX_LIST_ITEMS) {
      scroll_offset = _selected_index - (MAX_LIST_ITEMS - 1);
    }
  }

  // Render the contact list using macro
  RENDER_CONTACT_LIST(contacts, contact_count, _selected_index, scroll_offset, SCREEN_TOP_MARGIN, _rtc->getCurrentTime());

  display.endFrame();
  return 1000; // Refresh every second
}

bool ContactsScreen::handleInput(char c) {
  // Count valid contacts
  int contact_count = 0;
  for (int i = 0; i < UI_RECENT_LIST_SIZE; i++) {
    if (recent[i].name[0] != 0) contact_count++;
  }

  if (contact_count == 0) {
    if (c == KEY_CANCEL || c == KEY_LEFT) {
      _task->gotoHomeScreen();
      return true;
    }
    return false;
  }

  if (c == KEY_UP || c == KEY_PREV) {
    _selected_index--;
    if (_selected_index < 0) {
      _selected_index = contact_count - 1; // Wrap to bottom
    }
    return true;
  }

  if (c == KEY_DOWN || c == KEY_NEXT) {
    _selected_index++;
    if (_selected_index >= contact_count) {
      _selected_index = 0; // Wrap to top
    }
    return true;
  }

  if (c == KEY_CANCEL || c == KEY_LEFT) {
    _task->gotoHomeScreen();
    return true;
  }

  return false;
}
