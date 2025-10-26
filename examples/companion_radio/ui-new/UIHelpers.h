#pragma once

#include <helpers/ui/DisplayDriver.h>
#include "../MyMesh.h"

extern MyMesh the_mesh;

// Convert number to Roman numerals (up to 99) in lowercase
inline void toRomanNumeral(int num, char* buffer, size_t bufferSize) {
    if (num <= 0 || num > 99) {
        snprintf(buffer, bufferSize, "?");
        return;
    }

    buffer[0] = '\0';
    int pos = 0;

    // Tens place
    int tens = num / 10;
    if (tens == 9) {
        buffer[pos++] = 'x'; buffer[pos++] = 'c'; // xc = 90
    } else if (tens >= 5) {
        buffer[pos++] = 'l'; // l = 50
        for (int i = 0; i < tens - 5; i++) buffer[pos++] = 'x';
    } else if (tens == 4) {
        buffer[pos++] = 'x'; buffer[pos++] = 'l'; // xl = 40
    } else {
        for (int i = 0; i < tens; i++) buffer[pos++] = 'x';
    }

    // Ones place
    int ones = num % 10;
    if (ones == 9) {
        buffer[pos++] = 'i'; buffer[pos++] = 'x'; // ix = 9
    } else if (ones >= 5) {
        buffer[pos++] = 'v'; // v = 5
        for (int i = 0; i < ones - 5; i++) buffer[pos++] = 'i';
    } else if (ones == 4) {
        buffer[pos++] = 'i'; buffer[pos++] = 'v'; // iv = 4
    } else {
        for (int i = 0; i < ones; i++) buffer[pos++] = 'i';
    }

    buffer[pos] = '\0';
}

// Helper function to render battery widget on screen header
inline void renderHeaderWidgets(DisplayDriver& display, uint16_t batteryMilliVolts) {
    // Battery icon
    const int minMilliVolts = 3000;
    const int maxMilliVolts = 4200;
    int batteryPercentage = ((batteryMilliVolts - minMilliVolts) * 100) / (maxMilliVolts - minMilliVolts);
    if (batteryPercentage < 0) batteryPercentage = 0;
    if (batteryPercentage > 100) batteryPercentage = 100;

    const int batteryWidth = 32;
    const int batteryHeight = 14;
    const int batteryX = display.width() - batteryWidth - 5;
    const int batteryY = 0;

    display.setColor(DisplayDriver::GREEN);
    display.drawRect(batteryX, batteryY, batteryWidth, batteryHeight);
    display.fillRect(batteryX + batteryWidth, batteryY + (batteryHeight / 4), 3, batteryHeight / 2);
    int fillWidth = (batteryPercentage * (batteryWidth - 4)) / 100;
    display.fillRect(batteryX + 2, batteryY + 2, fillWidth, batteryHeight - 4);

    // Nearby count widget REMOVED - keeping only battery indicator
}

// Macro to draw screen header with title and battery indicator
#define DRAW_SCREEN_HEADER(title, task) \
  display.setTextSize(1); \
  display.setColor(DisplayDriver::LIGHT); \
  display.setCursor(0, 0); \
  display.print(title); \
  renderHeaderWidgets(display, (task)->getBattMilliVolts()); \
  display.setColor(DisplayDriver::LIGHT); \
  for (int dx = 0; dx < display.width(); dx += 3) { \
    display.fillRect(dx, 18, 1, 1); \
  }
