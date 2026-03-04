#pragma once

#include <helpers/ui/DisplayDriver.h>
#include "../MyMesh.h"
#include <time.h>
#include <RTClib.h>

extern MyMesh the_mesh;

// Helper function to render battery widget and time on screen header
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

    // Time display (hh:mm) - left of battery
    time_t now = time(NULL);
    if (now > 0) {
        DateTime dt = DateTime(now);
        char timeBuf[6];
        sprintf(timeBuf, "%02d:%02d", dt.hour(), dt.minute());
        display.setColor(DisplayDriver::GREEN);
        display.setTextSize(1);
        display.drawTextRightAlign(batteryX - 5, 0, timeBuf);
    }
}

// Optimized dotted line drawing - batches horizontal segments
inline void drawDottedHLine(DisplayDriver& display, int x, int y, int width, int spacing = 3) {
  // Instead of drawing individual pixels, draw small rectangles in a pattern
  // This reduces the number of display driver calls significantly
  for (int dx = 0; dx < width; dx += spacing) {
    display.fillRect(x + dx, y, 1, 1);
  }
}

// Optimized dotted line drawing - vertical
inline void drawDottedVLine(DisplayDriver& display, int x, int y, int height, int spacing = 3) {
  for (int dy = 0; dy < height; dy += spacing) {
    display.fillRect(x, y + dy, 1, 1);
  }
}

// Optimized dotted rectangle border - draws all four sides efficiently
inline void drawDottedRect(DisplayDriver& display, int x, int y, int width, int height, int spacing = 3) {
  // Top and bottom edges - horizontal
  drawDottedHLine(display, x, y, width, spacing);
  drawDottedHLine(display, x, y + height - 1, width, spacing);

  // Left and right edges - vertical (skip corners to avoid overdraw)
  if (height > 2) {
    drawDottedVLine(display, x, y + 1, height - 2, spacing);
    drawDottedVLine(display, x + width - 1, y + 1, height - 2, spacing);
  }
}

// Optimized dotted line between two points (Bresenham-like algorithm)
inline void drawDottedLine(DisplayDriver& display, int x0, int y0, int x1, int y1, int spacing = 3) {
  int dx = abs(x1 - x0);
  int dy = abs(y1 - y0);
  int sx = x0 < x1 ? 1 : -1;
  int sy = y0 < y1 ? 1 : -1;
  int err = dx - dy;
  int step_count = 0;

  while (true) {
    // Draw pixel every 'spacing' steps
    if (step_count % spacing == 0) {
      display.fillRect(x0, y0, 1, 1);
    }

    if (x0 == x1 && y0 == y1) break;

    int e2 = 2 * err;
    if (e2 > -dy) {
      err -= dy;
      x0 += sx;
    }
    if (e2 < dx) {
      err += dx;
      y0 += sy;
    }
    step_count++;
  }
}

// Macro to draw screen header with title, time, and battery indicator (optimized)
#define DRAW_SCREEN_HEADER(title, task) \
  display.setTextSize(1); \
  display.setColor(DisplayDriver::LIGHT); \
  display.setCursor(0, 0); \
  display.print(title); \
  renderHeaderWidgets(display, (task)->getBattMilliVolts()); \
  display.setColor(DisplayDriver::LIGHT); \
  drawDottedHLine(display, 0, 18, display.width(), 3);
