#include "TelemetryScreen.h"
#include "UITask.h"
#include "UIHelpers.h"
#include "helpers/ui/UIStrings.h"
#include <CayenneLPP.h>
#include <helpers/sensors/LPPDataHelpers.h>

static constexpr int SCREEN_TOP_MARGIN = 20;
static constexpr int SCREEN_BOTTOM_MARGIN = 5;

TelemetryScreen::TelemetryScreen(UITask* task, SensorManager* sensors, NodePrefs* node_prefs)
  : _task(task), _sensors(sensors), _node_prefs(node_prefs) {
}

int TelemetryScreen::render(DisplayDriver& display) {
  display.clear();
  display.startFrame();

  // Draw header
  DRAW_SCREEN_HEADER(TR(STR_TELEMETRY), _task);

  // Query sensor data using CayenneLPP
  CayenneLPP telemetry(64);
  _sensors->querySensors(TELEM_PERM_ENVIRONMENT, telemetry);

  // Parse telemetry data
  LPPReader reader(telemetry.getBuffer(), telemetry.getSize());

  float temp = -127.0;
  float humidity = -1.0;
  float pressure = 0.0;
  bool has_temp = false;
  bool has_humidity = false;
  bool has_pressure = false;

  uint8_t channel, type;
  while (reader.readHeader(channel, type)) {
    switch (type) {
      case LPP_TEMPERATURE:
        reader.readTemperature(temp);
        has_temp = true;
        break;
      case LPP_RELATIVE_HUMIDITY:
        reader.readRelativeHumidity(humidity);
        has_humidity = true;
        break;
      case LPP_BAROMETRIC_PRESSURE:
        reader.readPressure(pressure);
        has_pressure = true;
        break;
      default:
        reader.skipData(type);
        break;
    }
  }

  // Display telemetry data
  display.setColor(DisplayDriver::GREEN);
  int line_height = display.getTextHeight("A");
  int line_spacing = line_height + 2;
  int y = SCREEN_TOP_MARGIN + (line_height / 2);
  char buf[50];

  // Battery
  uint16_t batt_mv = _task->getBattMilliVolts();
  int batt_percent = map(batt_mv, 3300, 4200, 0, 100);
  batt_percent = constrain(batt_percent, 0, 100);

  display.drawTextLeftAlign(0, y, TR(STR_BATTERY));
  display.setColor(DisplayDriver::LIGHT);
  char unit_pct[8];
  strcpy(unit_pct, TR(STR_UNIT_PERCENT));
  sprintf(buf, "%d%s (%dmV)", batt_percent, unit_pct, batt_mv);
  display.drawTextRightAlign(display.width() - 1, y, buf);

  // Temperature
  if (has_temp && temp != -127.0) {
    y += line_spacing;
    display.setColor(DisplayDriver::GREEN);
    display.drawTextLeftAlign(0, y, TR(STR_TEMPERATURE));
    display.setColor(DisplayDriver::LIGHT);
    char unit_c[8];
    strcpy(unit_c, TR(STR_UNIT_CELSIUS));
    sprintf(buf, "%.1f%s", temp, unit_c);
    display.drawTextRightAlign(display.width() - 1, y, buf);
  }

  // Humidity
  if (has_humidity && humidity >= 0) {
    y += line_spacing;
    display.setColor(DisplayDriver::GREEN);
    display.drawTextLeftAlign(0, y, TR(STR_HUMIDITY));
    display.setColor(DisplayDriver::LIGHT);
    strcpy(unit_pct, TR(STR_UNIT_PERCENT));
    sprintf(buf, "%.0f%s", humidity, unit_pct);
    display.drawTextRightAlign(display.width() - 1, y, buf);
  }

  // Pressure
  if (has_pressure && pressure > 0) {
    y += line_spacing;
    display.setColor(DisplayDriver::GREEN);
    display.drawTextLeftAlign(0, y, TR(STR_PRESSURE));
    display.setColor(DisplayDriver::LIGHT);
    char unit_hpa[8];
    strcpy(unit_hpa, TR(STR_UNIT_HPA));
    sprintf(buf, "%.0f %s", pressure, unit_hpa);
    display.drawTextRightAlign(display.width() - 1, y, buf);
  }

  // Location (GPS or manually set)
  LocationProvider* nmea = _sensors->getLocationProvider();
  bool has_gps_location = false;
  double lat = 0, lon = 0;

  if (nmea != NULL && nmea->isValid()) {
    // Get GPS location
    lat = nmea->getLatitude() / 1000000.0;
    lon = nmea->getLongitude() / 1000000.0;
    has_gps_location = true;
  } else if (_sensors->node_lat != 0 || _sensors->node_lon != 0) {
    // Fallback to manually set coordinates
    lat = _sensors->node_lat;
    lon = _sensors->node_lon;
    has_gps_location = true;
  }

  if (has_gps_location) {
    y += line_spacing;
    display.setColor(DisplayDriver::GREEN);
    display.drawTextLeftAlign(0, y, TR(STR_LATITUDE));
    display.setColor(DisplayDriver::LIGHT);
    sprintf(buf, "%.5f", lat);  // 5 decimal places = ~1m accuracy
    display.drawTextRightAlign(display.width() - 1, y, buf);

    y += line_spacing;
    display.setColor(DisplayDriver::GREEN);
    display.drawTextLeftAlign(0, y, TR(STR_LONGITUDE));
    display.setColor(DisplayDriver::LIGHT);
    sprintf(buf, "%.5f", lon);  // 5 decimal places = ~1m accuracy
    display.drawTextRightAlign(display.width() - 1, y, buf);
  }

  // Show "No sensors" if only battery data
  if (!has_temp && !has_humidity && !has_pressure && !has_gps_location) {
    y += line_spacing + 10;
    display.setColor(DisplayDriver::LIGHT);
    display.setTextSize(1);
    display.drawTextCentered(display.width() / 2, y, TR(STR_NO_ADDITIONAL_SENSORS));
    display.setTextSize(1);
    y += line_spacing;
    display.drawTextCentered(display.width() / 2, y, TR(STR_DETECTED));
  }

  return 30000; // Refresh every 30 seconds
}

bool TelemetryScreen::handleInput(char c) {
  if (c == KEY_CANCEL || c == KEY_LEFT) {
    // Back to reports
    _task->gotoReportsScreen();
    return true;
  }

  return false;
}

void TelemetryScreen::poll() {
  // Nothing to poll
}
