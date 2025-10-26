#include "GPSScreen.h"
#include "UITask.h"
#include "UIHelpers.h"

static constexpr int SCREEN_TOP_MARGIN = 20;
static constexpr int SCREEN_BOTTOM_MARGIN = 5;

GPSScreen::GPSScreen(UITask* task, SensorManager* sensors)
  : _task(task), _sensors(sensors) {
}

int GPSScreen::render(DisplayDriver& display) {
  display.clear();
  display.startFrame();

  // Draw header
  DRAW_SCREEN_HEADER("GPS", _task);

  LocationProvider* nmea = _sensors->getLocationProvider();
  bool gps_on = _task->getGPSState();

  int available_height = display.height() - SCREEN_TOP_MARGIN - SCREEN_BOTTOM_MARGIN;
  int centerY = SCREEN_TOP_MARGIN + (available_height / 2);

  if (!gps_on) {
    // GPS is OFF - show centered message
    display.setColor(DisplayDriver::LIGHT);
    display.setTextSize(2);
    display.drawTextCentered(display.width() / 2, centerY - 10, "[ GPS OFF ]");
    display.setTextSize(1);
    display.drawTextCentered(display.width() / 2, centerY + 10, "Enable in Settings");
  } else if (nmea == NULL) {
    // GPS is ON but can't access
    display.setColor(DisplayDriver::LIGHT);
    display.setTextSize(2);
    display.drawTextCentered(display.width() / 2, centerY - 10, "GPS: ERROR");
    display.setTextSize(1);
    display.drawTextCentered(display.width() / 2, centerY + 10, "Can't access GPS");
  } else {
    // GPS is ON and working - show data
    display.setColor(DisplayDriver::LIGHT);
    int line_height = display.getTextHeight("A");
    int line_spacing = line_height + 2;
    int num_lines = 5;
    int total_height = (num_lines * line_height) + ((num_lines - 1) * 2);  // 5 text heights + 4 gaps of 2px
    int y = SCREEN_TOP_MARGIN + ((available_height - total_height) / 2) + (line_height / 2);
    char buf[50];

    // Status line with clear indicator
    display.setColor(DisplayDriver::GREEN);
    display.drawTextLeftAlign(0, y, nmea->isValid() ? "GPS [FIX]" : "GPS [SEARCH]");
    display.setColor(DisplayDriver::LIGHT);
    sprintf(buf, "%ds", nmea->satellitesCount());
    display.drawTextRightAlign(display.width() - 1, y, buf);

    y = y + line_spacing;
    display.setColor(DisplayDriver::GREEN);
    display.drawTextLeftAlign(0, y, "lat");
    display.setColor(DisplayDriver::LIGHT);
    sprintf(buf, "%.5f", nmea->getLatitude() / 1000000.);
    display.drawTextRightAlign(display.width() - 1, y, buf);

    y = y + line_spacing;
    display.setColor(DisplayDriver::GREEN);
    display.drawTextLeftAlign(0, y, "lon");
    display.setColor(DisplayDriver::LIGHT);
    sprintf(buf, "%.5f", nmea->getLongitude() / 1000000.);
    display.drawTextRightAlign(display.width() - 1, y, buf);

    y = y + line_spacing;
    display.setColor(DisplayDriver::GREEN);
    display.drawTextLeftAlign(0, y, "alt");
    display.setColor(DisplayDriver::LIGHT);
    sprintf(buf, "%.1fm", nmea->getAltitude() / 1000.);
    display.drawTextRightAlign(display.width() - 1, y, buf);

    y = y + line_spacing;
    // Get actual HDOP-based accuracy in meters
    float accuracy = nmea->getAccuracy();

    display.setColor(DisplayDriver::GREEN);
    display.drawTextLeftAlign(0, y, "acc");
    display.setColor(DisplayDriver::LIGHT);
    sprintf(buf, "%.0fm", accuracy);
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
