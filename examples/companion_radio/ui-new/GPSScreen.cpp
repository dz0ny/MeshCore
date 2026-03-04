#include "GPSScreen.h"
#include "UITask.h"
#include "UIHelpers.h"
#include "helpers/ui/UIStrings.h"

static constexpr int SCREEN_TOP_MARGIN = 20;
static constexpr int SCREEN_BOTTOM_MARGIN = 5;

GPSScreen::GPSScreen(UITask* task, SensorManager* sensors, NodePrefs* node_prefs, mesh::RTCClock* rtc)
  : _task(task), _sensors(sensors), _node_prefs(node_prefs), _rtc(rtc) {
}

int GPSScreen::render(DisplayDriver& display) {
  display.clear();
  display.startFrame();

  // Draw header
  DRAW_SCREEN_HEADER(TR(STR_GPS), _task);

  LocationProvider* nmea = _sensors->getLocationProvider();
  bool gps_on = _task->getGPSState();

  int available_height = display.height() - SCREEN_TOP_MARGIN - SCREEN_BOTTOM_MARGIN;
  int centerY = SCREEN_TOP_MARGIN + (available_height / 2);

  if (!gps_on) {
    // GPS is OFF - show centered message
    display.setColor(DisplayDriver::LIGHT);
    display.setTextSize(2);
    display.drawTextCentered(display.width() / 2, centerY - 10, TR(STR_GPS_OFF));
    display.setTextSize(1);
    display.drawTextCentered(display.width() / 2, centerY + 10, TR(STR_ENABLE_IN_SETTINGS));
  } else if (nmea == NULL) {
    // GPS is ON but can't access
    display.setColor(DisplayDriver::LIGHT);
    display.setTextSize(2);
    display.drawTextCentered(display.width() / 2, centerY - 10, TR(STR_GPS_ERROR));
    display.setTextSize(1);
    display.drawTextCentered(display.width() / 2, centerY + 10, TR(STR_CANT_ACCESS_GPS));
  } else {
    // GPS is ON and working - show data
    display.setColor(DisplayDriver::LIGHT);
    int line_height = display.getTextHeight("A");
    int line_spacing = line_height + 2;
    int num_lines = 5;
    int total_height = (num_lines * line_height) + ((num_lines - 1) * 2);  // 5 text heights + 4 gaps of 2px
    int y = SCREEN_TOP_MARGIN + ((available_height - total_height) / 2) + (line_height / 2);
    char buf[50];
    char status_buf[80];

    // Status line with time since last fix
    long last_fix_time = nmea->getTimestamp();
    uint32_t current_time = _rtc->getCurrentTime();
    const char* status = nmea->isValid() ? TR(STR_GPS_FIX) : TR(STR_GPS_SEARCH);

    if (nmea->isValid() && last_fix_time > 0 && current_time > 0) {
      long elapsed = current_time - last_fix_time;
      if (elapsed < 60) {
        sprintf(status_buf, "%s (%lds)", status, elapsed);
      } else if (elapsed < 3600) {
        sprintf(status_buf, "%s (%ldm)", status, elapsed / 60);
      } else {
        sprintf(status_buf, "%s (%ldh)", status, elapsed / 3600);
      }
    } else {
      strcpy(status_buf, status);
    }

    display.setColor(DisplayDriver::GREEN);
    display.drawTextLeftAlign(0, y, status_buf);
    display.setColor(DisplayDriver::LIGHT);
    sprintf(buf, "%d", nmea->satellitesCount());
    display.drawTextRightAlign(display.width() - 1, y, buf);

    y = y + line_spacing;
    display.setColor(DisplayDriver::GREEN);
    display.drawTextLeftAlign(0, y, TR(STR_LAT));
    display.setColor(DisplayDriver::LIGHT);
    sprintf(buf, "%.5f", nmea->getLatitude() / 1000000.);
    display.drawTextRightAlign(display.width() - 1, y, buf);

    y = y + line_spacing;
    display.setColor(DisplayDriver::GREEN);
    display.drawTextLeftAlign(0, y, TR(STR_LON));
    display.setColor(DisplayDriver::LIGHT);
    sprintf(buf, "%.5f", nmea->getLongitude() / 1000000.);
    display.drawTextRightAlign(display.width() - 1, y, buf);

    y = y + line_spacing;
    display.setColor(DisplayDriver::GREEN);
    display.drawTextLeftAlign(0, y, TR(STR_ALT));
    display.setColor(DisplayDriver::LIGHT);
    char unit_m[8];
    strcpy(unit_m, TR(STR_UNIT_MINUTES));
    sprintf(buf, "%.1f%s", nmea->getAltitude() / 1000., unit_m);
    display.drawTextRightAlign(display.width() - 1, y, buf);

    y = y + line_spacing;
    // Get actual HDOP-based accuracy in meters
    float accuracy = nmea->getAccuracy();

    display.setColor(DisplayDriver::GREEN);
    display.drawTextLeftAlign(0, y, TR(STR_ACC));
    display.setColor(DisplayDriver::LIGHT);
    strcpy(unit_m, TR(STR_UNIT_MINUTES));
    sprintf(buf, "%.0f%s", accuracy, unit_m);
    display.drawTextRightAlign(display.width() - 1, y, buf);
  }

  return 30000; // Refresh every 30 seconds
}

bool GPSScreen::handleInput(char c) {
  if (c == KEY_CANCEL || c == KEY_LEFT) {
    // Back to reports
    _task->gotoReportsScreen();
    return true;
  }

  return false;
}

void GPSScreen::poll() {
  // Nothing to poll
}
