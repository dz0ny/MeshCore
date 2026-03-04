#include "SplashScreen.h"
#include "UITask.h"
#include "icons.h"
#include "target.h"
#include <string.h>

#ifndef BOOT_SCREEN_MILLIS
  #define BOOT_SCREEN_MILLIS   3000   // 3 seconds
#endif

SplashScreen::SplashScreen(UITask* task) : _task(task) {
  // strip off dash and commit hash by changing dash to null terminator
  // e.g: v1.2.3-abcdef -> v1.2.3
  const char *ver = FIRMWARE_VERSION;
  const char *dash = strchr(ver, '-');

  int len = dash ? dash - ver : strlen(ver);
  if (len >= sizeof(_version_info)) len = sizeof(_version_info) - 1;
  memcpy(_version_info, ver, len);
  _version_info[len] = 0;

  dismiss_after = millis() + BOOT_SCREEN_MILLIS;
}

int SplashScreen::render(DisplayDriver& display) {
  // Calculate vertical center with margins
  const int SCREEN_TOP_MARGIN = 5;
  const int SCREEN_BOTTOM_MARGIN = 5;
  int available_height = display.height() - SCREEN_TOP_MARGIN - SCREEN_BOTTOM_MARGIN;
  int centerY = SCREEN_TOP_MARGIN + (available_height / 2);

  // meshcore logo
  display.setColor(DisplayDriver::BLUE);
  int logoWidth = 128;
  int logoHeight = 13;
  display.drawXbm((display.width() - logoWidth) / 2, centerY - 25, meshcore_logo, logoWidth, logoHeight);

  // version info
  display.setColor(DisplayDriver::LIGHT);
  display.setTextSize(2);
  display.drawTextCentered(display.width()/2, centerY - 5, _version_info);

  display.setTextSize(1);
  display.drawTextCentered(display.width()/2, centerY + 15, FIRMWARE_BUILD_DATE);

  return 1000;
}

void SplashScreen::poll() {
  if (millis() >= dismiss_after) {
    _task->gotoHomeScreen();
  }
}
