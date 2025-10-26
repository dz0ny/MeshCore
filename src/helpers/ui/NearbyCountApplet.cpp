#ifdef MESHTASTIC_INCLUDE_INKHUD

#include "NearbyCountApplet.h"
#include <stdio.h>

using namespace InkHUD;

NearbyCountApplet::NearbyCountApplet(DisplayDriver* driver, int16_t x, int16_t y, uint16_t width, uint16_t height)
    : count(0), display(driver), x(x), y(y), w(width), h(height)
{
    // Initialize with 0 devices
}

void NearbyCountApplet::updateCount(uint8_t deviceCount)
{
    // Clamp to 0-99 range (max 2 digits for display)
    if (deviceCount > 99) deviceCount = 99;
    count = deviceCount;
}

void NearbyCountApplet::render()
{
    // Clear the region beneath the widget
    display->setColor(DisplayDriver::LIGHT);
    display->fillRect(x, y, w, h);

    // Draw rectangle border with 1px solid line
    display->setColor(DisplayDriver::DARK);
    display->drawRect(x, y, w, h);

    // Add internal padding (2px from all edges)
    const int16_t padding = 2;
    const int16_t innerX = x + padding;
    const int16_t innerY = y + padding;
    const int16_t innerWidth = w - (padding * 2);
    const int16_t innerHeight = h - (padding * 2);

    // Draw globe/mesh network icon (approximately 10x10 pixels)
    const int16_t iconX = innerX + 1;
    const int16_t iconY = innerY + (innerHeight / 2) - 5;
    const int16_t centerX = iconX + 5;
    const int16_t centerY = iconY + 5;
    const int16_t radius = 5;

    display->setColor(DisplayDriver::DARK);

    // Draw circle outline (globe)
    // Top arc
    for (int px = -radius; px <= radius; px++) {
        int py = -radius;
        if (px >= -4 && px <= 4) {
            display->fillRect(centerX + px, centerY + py, 1, 1);
        }
    }
    // Bottom arc
    for (int px = -radius; px <= radius; px++) {
        int py = radius;
        if (px >= -4 && px <= 4) {
            display->fillRect(centerX + px, centerY + py, 1, 1);
        }
    }
    // Left arc
    for (int py = -radius; py <= radius; py++) {
        int px = -radius;
        if (py >= -4 && py <= 4) {
            display->fillRect(centerX + px, centerY + py, 1, 1);
        }
    }
    // Right arc
    for (int py = -radius; py <= radius; py++) {
        int px = radius;
        if (py >= -4 && py <= 4) {
            display->fillRect(centerX + px, centerY + py, 1, 1);
        }
    }

    // Draw horizontal line (equator)
    for (int px = -3; px <= 3; px++) {
        display->fillRect(centerX + px, centerY, 1, 1);
    }

    // Draw vertical line (meridian)
    for (int py = -3; py <= 3; py++) {
        display->fillRect(centerX, centerY + py, 1, 1);
    }

    // Draw 3 small dots in center area to represent mesh nodes
    display->fillRect(centerX - 2, centerY - 2, 1, 1);
    display->fillRect(centerX + 2, centerY - 1, 1, 1);
    display->fillRect(centerX, centerY + 2, 1, 1);

    // Draw count number to the right of the icon
    char countStr[4];
    if (count > 99) {
        snprintf(countStr, sizeof(countStr), "99");
    } else {
        snprintf(countStr, sizeof(countStr), "%d", count);
    }

    display->setTextSize(1);
    display->setColor(DisplayDriver::GREEN);

    // Position text to the right of the icon with small gap (inside padding)
    int16_t textX = iconX + 13;
    int16_t textY = innerY + (innerHeight / 2) - (display->getTextHeight(countStr) / 2);

    display->drawTextLeftAlign(textX, textY, countStr);
}

#endif // MESHTASTIC_INCLUDE_INKHUD
