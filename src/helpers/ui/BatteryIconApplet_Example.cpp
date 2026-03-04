/**
 * Example usage of BatteryIconApplet for ink/e-paper displays
 *
 * This file demonstrates how to integrate the BatteryIconApplet
 * with your existing display code.
 *
 * To enable this feature, add to your platformio.ini or build flags:
 *   -DMESHTASTIC_INCLUDE_INKHUD
 */

#ifdef MESHTASTIC_INCLUDE_INKHUD

#include "BatteryIconApplet.h"
#include "DisplayDriver.h"

// Example: Using BatteryIconApplet in a render function
void exampleRenderWithBatteryIcon(DisplayDriver& display, uint16_t batteryMilliVolts) {
    // Create the battery icon applet (positioned at top-right corner)
    InkHUD::BatteryIconApplet batteryIcon(
        &display,
        display.width() - 32,  // x position (32 pixels from right edge)
        2,                      // y position (2 pixels from top)
        30,                     // width
        15                      // height
    );

    // Update battery level from millivolts
    batteryIcon.updateBatteryMilliVolts(batteryMilliVolts);

    // Render the icon
    batteryIcon.render();
}

// Example: Replacing existing renderBatteryIndicator function
// You can replace your existing renderBatteryIndicator() with this:
void renderBatteryIndicator(DisplayDriver& display, uint16_t batteryMilliVolts) {
    static InkHUD::BatteryIconApplet* batteryIcon = nullptr;

    // Create icon on first call
    if (batteryIcon == nullptr) {
        batteryIcon = new InkHUD::BatteryIconApplet(
            &display,
            display.width() - 30 - 5,  // x: positioned near top-right
            0,                          // y: top of screen
            30,                         // width
            15                          // height
        );
    }

    // Update and render
    batteryIcon->updateBatteryMilliVolts(batteryMilliVolts);
    batteryIcon->render();
}

// Example: Using with battery percentage directly
void exampleWithPercentage(DisplayDriver& display, uint8_t batteryPercent) {
    InkHUD::BatteryIconApplet batteryIcon(&display, 100, 5, 30, 15);
    batteryIcon.updateBatteryLevel(batteryPercent);
    batteryIcon.render();
}

#endif // MESHTASTIC_INCLUDE_INKHUD
