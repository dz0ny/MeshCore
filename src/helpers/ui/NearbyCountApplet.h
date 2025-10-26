#pragma once

#ifdef MESHTASTIC_INCLUDE_INKHUD

#include "DisplayDriver.h"
#include <stdint.h>

namespace InkHUD {

/**
 * Nearby devices count applet for ink/e-paper displays
 * Renders a small icon with nearby device count
 */
class NearbyCountApplet {
private:
    uint8_t count;       // Number of nearby devices
    DisplayDriver* display;
    int16_t x, y;        // Position on display
    uint16_t w, h;       // Width and height of widget

public:
    /**
     * Constructor
     * @param driver Pointer to display driver
     * @param x X position of widget
     * @param y Y position of widget
     * @param width Width of widget (default: 30)
     * @param height Height of widget (default: 15)
     */
    NearbyCountApplet(DisplayDriver* driver, int16_t x, int16_t y, uint16_t width = 30, uint16_t height = 15);

    /**
     * Update nearby device count
     * @param deviceCount Number of nearby devices (0-99)
     */
    void updateCount(uint8_t deviceCount);

    /**
     * Render the widget to the display
     */
    void render();

    /**
     * Get current device count
     */
    uint8_t getCount() const { return count; }

    /**
     * Get widget width
     */
    uint16_t width() const { return w; }

    /**
     * Get widget height
     */
    uint16_t height() const { return h; }
};

} // namespace InkHUD

#endif // MESHTASTIC_INCLUDE_INKHUD
