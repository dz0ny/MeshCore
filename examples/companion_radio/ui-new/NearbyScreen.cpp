#include "NearbyScreen.h"
#include "UITask.h"
#include "../MyMesh.h"
#include <math.h>

extern MyMesh the_mesh;

static constexpr int SCREEN_TOP_MARGIN = 20;
static constexpr int SCREEN_BOTTOM_MARGIN = 18;

NearbyScreen::NearbyScreen(UITask* task, mesh::RTCClock* rtc, SensorManager* sensors)
  : _task(task),
    _rtc(rtc),
    _sensors(sensors),
    _selected_index(-1) {
}

void NearbyScreen::reset() {
  _selected_index = -1;
}

int NearbyScreen::render(DisplayDriver& display) {
  display.clear();
  display.startFrame();

  // Draw header
  display.setTextSize(1);
  display.setColor(DisplayDriver::LIGHT);
  display.setCursor(0, 0);
  display.print("Nearby Nodes");

  // Draw separator line
  display.setColor(DisplayDriver::LIGHT);
  for (int dx = 0; dx < display.width(); dx += 3) {
    display.fillRect(dx, 18, 1, 1);
  }

  AdvertPath recent[UI_RECENT_LIST_SIZE];
  the_mesh.getRecentlyHeard(recent, UI_RECENT_LIST_SIZE);

  display.setColor(DisplayDriver::GREEN);
  int line_height = display.getTextHeight("A");
  int line_spacing = line_height + 2;

  if (_selected_index >= 0 && _selected_index < UI_RECENT_LIST_SIZE && recent[_selected_index].name[0] != 0) {
    // Detail view with map
    auto a = &recent[_selected_index];
    LocationProvider* nmea = _sensors->getLocationProvider();
    bool has_gps = (nmea != NULL && nmea->isValid());

    if (has_gps) {
      // Draw map view
      display.setColor(DisplayDriver::LIGHT);
      double my_lat = nmea->getLatitude() / 1000000.0;
      double my_lon = nmea->getLongitude() / 1000000.0;

      // Try to find contact info for GPS data
      ContactInfo contact;
      bool found_contact = false;
      ContactsIterator iter;
      while (iter.hasNext(&the_mesh, contact)) {
        if (memcmp(contact.id.pub_key, a->pubkey_prefix, 7) == 0) {
          found_contact = true;
          break;
        }
      }

      if (found_contact && (contact.gps_lat != 0 || contact.gps_lon != 0)) {
        double contact_lat = contact.gps_lat / 1000000.0;
        double contact_lon = contact.gps_lon / 1000000.0;

        // Calculate center point between me and contact
        double center_lat = (my_lat + contact_lat) / 2.0;
        double center_lon = (my_lon + contact_lon) / 2.0;

        // Calculate distance for zoom
        double dlat = (contact_lat - my_lat) * 3.14159 / 180.0;
        double dlon = (contact_lon - my_lon) * 3.14159 / 180.0;
        double a_calc = sin(dlat/2) * sin(dlat/2) + cos(my_lat * 3.14159 / 180.0) * cos(contact_lat * 3.14159 / 180.0) * sin(dlon/2) * sin(dlon/2);
        double c = 2 * atan2(sqrt(a_calc), sqrt(1-a_calc));
        double distance_km = 6371 * c;

        // Auto zoom to fit both points
        double km_per_pixel = max(1.0, distance_km / (display.width() / 3));

        int center_x = display.width() / 2;
        int center_y = display.height() / 2;

        // Calculate my position on map
        double my_lat_offset_km = (my_lat - center_lat) * 111.0;
        double my_lon_offset_km = (my_lon - center_lon) * 111.0 * cos(center_lat * 3.14159 / 180.0);
        int my_x = center_x + (int)(my_lon_offset_km / km_per_pixel);
        int my_y = center_y - (int)(my_lat_offset_km / km_per_pixel);

        // Calculate contact position on map
        double contact_lat_offset_km = (contact_lat - center_lat) * 111.0;
        double contact_lon_offset_km = (contact_lon - center_lon) * 111.0 * cos(center_lat * 3.14159 / 180.0);
        int contact_x = center_x + (int)(contact_lon_offset_km / km_per_pixel);
        int contact_y = center_y - (int)(contact_lat_offset_km / km_per_pixel);

        // Draw dotted line between me and contact
        int dx = contact_x - my_x;
        int dy = contact_y - my_y;
        int steps = max(abs(dx), abs(dy));
        for (int i = 0; i <= steps; i += 3) {  // Draw every 3rd pixel for dotted line
          int x = my_x + (dx * i) / steps;
          int y = my_y + (dy * i) / steps;
          if (x >= 0 && x < display.width() && y >= SCREEN_TOP_MARGIN && y < display.height() - SCREEN_BOTTOM_MARGIN) {
            display.fillRect(x, y, 1, 1);
          }
        }

        // Draw me (solid square)
        display.fillRect(my_x - 2, my_y - 2, 5, 5);

        // Draw contact (hollow circle - approximate with rect)
        display.drawRect(contact_x - 2, contact_y - 2, 5, 5);

        // Show contact name at top
        display.setTextSize(1);
        display.setColor(DisplayDriver::YELLOW);
        char filtered_name[sizeof(a->name)];
        display.translateUTF8ToBlocks(filtered_name, a->name, sizeof(filtered_name));
        display.drawTextCentered(display.width() / 2, SCREEN_TOP_MARGIN, filtered_name);

        // Show distance at bottom
        char dist_buf[30];
        if (distance_km < 1.0) {
          sprintf(dist_buf, "%.0fm", distance_km * 1000);
        } else {
          sprintf(dist_buf, "%.1fkm", distance_km);
        }
        display.setColor(DisplayDriver::LIGHT);
        display.drawTextCentered(display.width() / 2, display.height() - SCREEN_BOTTOM_MARGIN - 5, dist_buf);
      } else {
        // No GPS data for contact
        display.setTextSize(2);
        int centerY = SCREEN_TOP_MARGIN + ((display.height() - SCREEN_TOP_MARGIN - SCREEN_BOTTOM_MARGIN) / 2);
        display.drawTextCentered(display.width() / 2, centerY - 10, "No GPS data");
        display.setTextSize(1);
        display.drawTextCentered(display.width() / 2, centerY + 10, "for this node");
      }
    } else {
      // No GPS fix
      display.setTextSize(2);
      int centerY = SCREEN_TOP_MARGIN + ((display.height() - SCREEN_TOP_MARGIN - SCREEN_BOTTOM_MARGIN) / 2);
      display.drawTextCentered(display.width() / 2, centerY - 10, "No GPS fix");
      display.setTextSize(1);
      display.drawTextCentered(display.width() / 2, centerY + 10, "Cannot show map");
    }

    // Footer for map view
    display.setColor(DisplayDriver::LIGHT);
    display.setTextSize(0);
    display.setCursor(2, display.height() - SCREEN_BOTTOM_MARGIN);
    display.print("Back: List");
  } else {
    // List view - show all nearby adverts
    LocationProvider* nmea = _sensors->getLocationProvider();
    bool has_gps = (nmea != NULL && nmea->isValid());
    double my_lat = 0, my_lon = 0;
    if (has_gps) {
      my_lat = nmea->getLatitude() / 1000000.0;
      my_lon = nmea->getLongitude() / 1000000.0;
    }

    // Count valid nodes
    int valid_count = 0;
    for (int i = 0; i < UI_RECENT_LIST_SIZE; i++) {
      if (recent[i].name[0] != 0) valid_count++;
    }

    if (valid_count == 0) {
      // No nearby nodes
      display.setTextSize(2);
      int centerY = SCREEN_TOP_MARGIN + ((display.height() - SCREEN_TOP_MARGIN - SCREEN_BOTTOM_MARGIN) / 2);
      display.setColor(DisplayDriver::LIGHT);
      display.drawTextCentered(display.width() / 2, centerY - 10, "No nodes");
      display.setTextSize(1);
      display.drawTextCentered(display.width() / 2, centerY + 10, "nearby");
    } else {
      int total_height = valid_count * line_spacing;
      int available_height = display.height() - SCREEN_TOP_MARGIN - SCREEN_BOTTOM_MARGIN;
      int y = SCREEN_TOP_MARGIN + ((available_height - total_height) / 2) + (line_height / 2);

      int item_idx = 0;
      for (int i = 0; i < UI_RECENT_LIST_SIZE; i++) {
        auto a = &recent[i];
        if (a->name[0] == 0) continue;

        int secs = _rtc->getCurrentTime() - a->recv_timestamp;
        char time_buf[20];
        if (secs < 60) {
          sprintf(time_buf, "%ds", secs);
        } else if (secs < 60*60) {
          sprintf(time_buf, "%dm", secs / 60);
        } else {
          sprintf(time_buf, "%dh", secs / (60*60));
        }

        // Calculate distance if we have GPS and contact has GPS
        char dist_buf[20] = "";
        if (has_gps) {
          ContactInfo contact;
          bool found_contact = false;
          ContactsIterator iter;
          while (iter.hasNext(&the_mesh, contact)) {
            if (memcmp(contact.id.pub_key, a->pubkey_prefix, 7) == 0) {
              found_contact = true;
              break;
            }
          }

          if (found_contact && (contact.gps_lat != 0 || contact.gps_lon != 0)) {
            double contact_lat = contact.gps_lat / 1000000.0;
            double contact_lon = contact.gps_lon / 1000000.0;

            double dlat = (contact_lat - my_lat) * 3.14159 / 180.0;
            double dlon = (contact_lon - my_lon) * 3.14159 / 180.0;
            double a_calc = sin(dlat/2) * sin(dlat/2) + cos(my_lat * 3.14159 / 180.0) * cos(contact_lat * 3.14159 / 180.0) * sin(dlon/2) * sin(dlon/2);
            double c = 2 * atan2(sqrt(a_calc), sqrt(1-a_calc));
            double distance_km = 6371 * c;

            if (distance_km < 1.0) {
              sprintf(dist_buf, " %.0fm", distance_km * 1000);
            } else {
              sprintf(dist_buf, " %.1fkm", distance_km);
            }
          }
        }

        // Format: "name  time dist"
        char line_buf[50];
        snprintf(line_buf, sizeof(line_buf), "%s%s", time_buf, dist_buf);
        int info_width = display.getTextWidth(line_buf);
        int max_name_width = display.width() - info_width - 10;  // Extra space for selection arrow

        // Draw selection arrow
        if (item_idx == _selected_index) {
          display.setColor(DisplayDriver::YELLOW);
          display.drawTextLeftAlign(0, y, ">");
        }

        display.setColor(DisplayDriver::GREEN);
        char filtered_name[sizeof(a->name)];
        display.translateUTF8ToBlocks(filtered_name, a->name, sizeof(filtered_name));
        display.drawTextEllipsized(8, y, max_name_width, filtered_name);
        display.drawTextRightAlign(display.width() - 1, y, line_buf);

        y += line_spacing;
        item_idx++;
      }
    }

    // Footer for list view
    display.setColor(DisplayDriver::LIGHT);
    display.setTextSize(0);
    display.setCursor(2, display.height() - SCREEN_BOTTOM_MARGIN);
    if (valid_count > 0) {
      display.print("Up/Down:Select Enter:Map");
    } else {
      display.print("Back: Home");
    }
  }

  return 1000; // Refresh every second
}

bool NearbyScreen::handleInput(char c) {
  AdvertPath recent[UI_RECENT_LIST_SIZE];
  the_mesh.getRecentlyHeard(recent, UI_RECENT_LIST_SIZE);

  // Count valid nodes
  int valid_count = 0;
  for (int i = 0; i < UI_RECENT_LIST_SIZE; i++) {
    if (recent[i].name[0] != 0) valid_count++;
  }

  if (_selected_index >= 0) {
    // In detail/map view - back button returns to list
    if (c == KEY_LEFT || c == KEY_CANCEL) {
      _selected_index = -1;
      return true;
    }
  } else {
    // In list view - up/down to select node
    if (valid_count > 0) {
      if (c == KEY_UP || c == KEY_PREV) {
        if (_selected_index < 0) {
          // Find first valid node
          for (int i = 0; i < UI_RECENT_LIST_SIZE; i++) {
            if (recent[i].name[0] != 0) {
              _selected_index = i;
              break;
            }
          }
        } else {
          // Move to previous valid node
          int start = _selected_index;
          for (int i = 1; i <= UI_RECENT_LIST_SIZE; i++) {
            int idx = (start - i + UI_RECENT_LIST_SIZE) % UI_RECENT_LIST_SIZE;
            if (recent[idx].name[0] != 0) {
              _selected_index = idx;
              break;
            }
          }
        }
        return true;
      }

      if (c == KEY_DOWN || c == KEY_NEXT) {
        if (_selected_index < 0) {
          // Find first valid node
          for (int i = 0; i < UI_RECENT_LIST_SIZE; i++) {
            if (recent[i].name[0] != 0) {
              _selected_index = i;
              break;
            }
          }
        } else {
          // Move to next valid node
          int start = _selected_index;
          for (int i = 1; i <= UI_RECENT_LIST_SIZE; i++) {
            int idx = (start + i) % UI_RECENT_LIST_SIZE;
            if (recent[idx].name[0] != 0) {
              _selected_index = idx;
              break;
            }
          }
        }
        return true;
      }

      if (c == KEY_ENTER || c == KEY_SELECT) {
        // If no selection yet, select first node
        if (_selected_index < 0) {
          for (int i = 0; i < UI_RECENT_LIST_SIZE; i++) {
            if (recent[i].name[0] != 0) {
              _selected_index = i;
              break;
            }
          }
        }
        // Selection already made, stays in map view now
        return true;
      }
    }

    // Back button from list view
    if (c == KEY_CANCEL || c == KEY_LEFT) {
      _task->gotoHomeScreen();
      return true;
    }
  }

  return false;
}

void NearbyScreen::poll() {
  // Nothing to poll
}
