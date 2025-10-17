#pragma once

#ifdef MESHTASTIC_INCLUDE_INKHUD

#include "DisplayDriver.h"
#include <stdint.h>

namespace InkHUD {

/**
 * Battery icon applet for ink/e-paper displays
 * Renders a battery icon with charge level indication
 */
class BatteryIconApplet {
private:
    uint8_t socRounded;  // State of charge, rounded to nearest 10%
    DisplayDriver* display;
    int16_t x, y;        // Position on display
    uint16_t w, h;       // Width and height of icon

public:
    /**
     * Constructor
     * @param driver Pointer to display driver
     * @param x X position of icon
     * @param y Y position of icon
     * @param width Width of icon (default: 30)
     * @param height Height of icon (default: 15)
     */
    BatteryIconApplet(DisplayDriver* driver, int16_t x, int16_t y, uint16_t width = 30, uint16_t height = 15);

    /**
     * Update battery charge percentage
     * @param batteryPercent Battery charge percentage (0-100)
     */
    void updateBatteryLevel(uint8_t batteryPercent);

    /**
     * Update battery level from millivolts
     * @param batteryMilliVolts Battery voltage in millivolts
     */
    void updateBatteryMilliVolts(int batteryMilliVolts);

    /**
     * Render the battery icon to the display
     */
    void render();

    /**
     * Get current battery percentage (rounded to nearest 10%)
     */
    uint8_t getBatteryPercent() const { return socRounded; }

    /**
     * Get icon width
     */
    uint16_t width() const { return w; }

    /**
     * Get icon height
     */
    uint16_t height() const { return h; }

private:
    /**
     * Draw a hatched region (diagonal lines pattern)
     * @param l Left coordinate
     * @param t Top coordinate
     * @param width Width of region
     * @param height Height of region
     * @param spacing Spacing between hatch lines
     * @param color Color to draw (typically BLACK)
     */
    void hatchRegion(int16_t l, int16_t t, uint16_t width, uint16_t height, int16_t spacing, DisplayDriver::Color color);
};

} // namespace InkHUD

#endif // MESHTASTIC_INCLUDE_INKHUD
