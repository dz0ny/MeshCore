#ifdef MESHTASTIC_INCLUDE_INKHUD

#include "BatteryIconApplet.h"
#include <algorithm>

using namespace InkHUD;

BatteryIconApplet::BatteryIconApplet(DisplayDriver* driver, int16_t x, int16_t y, uint16_t width, uint16_t height)
    : socRounded(0), display(driver), x(x), y(y), w(width), h(height)
{
    // Initialize with 0% charge
}

void BatteryIconApplet::updateBatteryLevel(uint8_t batteryPercent)
{
    // Clamp to 0-100 range
    if (batteryPercent > 100) batteryPercent = 100;

    // Round to nearest 10%
    uint8_t newSocRounded = ((batteryPercent + 5) / 10) * 10;

    // Update stored value
    socRounded = newSocRounded;
}

void BatteryIconApplet::updateBatteryMilliVolts(int batteryMilliVolts)
{
    // Convert millivolts to percentage
    // LiPo battery: 3.0V (empty) to 4.2V (full)
    const int minMilliVolts = 3000;
    const int maxMilliVolts = 4200;

    int percentage = ((batteryMilliVolts - minMilliVolts) * 100) / (maxMilliVolts - minMilliVolts);

    // Clamp to 0-100 range
    if (percentage < 0) percentage = 0;
    if (percentage > 100) percentage = 100;

    updateBatteryLevel((uint8_t)percentage);
}

void BatteryIconApplet::hatchRegion(int16_t l, int16_t t, uint16_t width, uint16_t height, int16_t spacing, DisplayDriver::Color color)
{
    if (width == 0 || height == 0) return;

    display->setColor(color);

    // Draw diagonal lines from top-left to bottom-right
    for (int16_t offset = -(int16_t)height; offset < (int16_t)width; offset += spacing) {
        int16_t x1 = l + offset;
        int16_t y1 = t;
        int16_t x2 = l + offset + height;
        int16_t y2 = t + height;

        // Clip to region bounds
        if (x1 < l) {
            y1 += (l - x1);
            x1 = l;
        }
        if (x2 > l + (int16_t)width) {
            y2 -= (x2 - (l + (int16_t)width));
            x2 = l + width;
        }

        // Draw line (simplified - draws as series of pixels since DisplayDriver doesn't have drawLine)
        // For proper implementation, you might want to add drawLine to DisplayDriver
        int16_t dx = x2 - x1;
        int16_t dy = y2 - y1;
        int16_t steps = (dx > dy) ? dx : dy;

        if (steps > 0) {
            float xInc = (float)dx / steps;
            float yInc = (float)dy / steps;
            float xf = x1;
            float yf = y1;

            for (int16_t i = 0; i <= steps; i++) {
                display->fillRect((int16_t)xf, (int16_t)yf, 1, 1);
                xf += xInc;
                yf += yInc;
            }
        }
    }
}

void BatteryIconApplet::render()
{
    // Clear the region beneath the icon
    display->setColor(DisplayDriver::LIGHT);
    display->fillRect(x, y, w, h);

    // Vertical centerline
    const int16_t m = y + (h / 2);

    // =====================
    // Draw battery outline
    // =====================

    // Positive terminal "bump"
    const int16_t bumpL = x;
    const uint16_t bumpH = h / 2;
    const int16_t bumpT = m - (bumpH / 2);
    constexpr uint16_t bumpW = 2;

    display->setColor(DisplayDriver::DARK);
    display->fillRect(bumpL, bumpT, bumpW, bumpH);

    // Main body of battery
    const int16_t bodyL = bumpL + bumpW;
    const int16_t bodyT = y;
    const int16_t bodyH = h;
    const int16_t bodyW = w - bumpW;

    display->setColor(DisplayDriver::DARK);
    display->drawRect(bodyL, bodyT, bodyW, bodyH);

    // Erase join between bump and body (draw white line)
    display->setColor(DisplayDriver::LIGHT);
    for (int16_t i = 0; i < bumpH; i++) {
        display->fillRect(bodyL, bumpT + i, 1, 1);
    }

    // ===================
    // Draw battery level
    // ===================

    constexpr int16_t slicePad = 2;
    const int16_t sliceL = bodyL + slicePad;
    const int16_t sliceT = bodyT + slicePad;
    const uint16_t sliceH = bodyH - (slicePad * 2);
    uint16_t sliceW = bodyW - (slicePad * 2);

    // Apply percentage to width
    sliceW = (sliceW * socRounded) / 100;

    if (sliceW > 0) {
        // Draw hatched pattern for battery fill
        hatchRegion(sliceL, sliceT, sliceW, sliceH, 2, DisplayDriver::DARK);

        // Draw outline around filled region
        display->setColor(DisplayDriver::DARK);
        display->drawRect(sliceL, sliceT, sliceW, sliceH);
    }
}

#endif // MESHTASTIC_INCLUDE_INKHUD
