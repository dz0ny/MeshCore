#include "RadioStatsScreen.h"
#include "UITask.h"
#include "UIHelpers.h"
#include "../MyMesh.h"
#include "target.h"

extern MyMesh the_mesh;

static constexpr int SCREEN_TOP_MARGIN = 20;
static constexpr int SCREEN_BOTTOM_MARGIN = 5;

RadioStatsScreen::RadioStatsScreen(UITask* task, SensorManager* sensors, NodePrefs* node_prefs)
  : _task(task),
    _sensors(sensors),
    _node_prefs(node_prefs) {
}

int RadioStatsScreen::render(DisplayDriver& display) {
  display.clear();
  display.startFrame();

  // Draw header
  DRAW_SCREEN_HEADER("Radio Stats", _task);

  // Draw radio statistics
  display.setColor(DisplayDriver::YELLOW);
  display.setTextSize(1);
  int line_height = display.getTextHeight("A");
  int line_spacing = line_height + 2;

  int available_height = display.height() - SCREEN_TOP_MARGIN - SCREEN_BOTTOM_MARGIN;
  int total_height = 4 * line_spacing;
  int y = SCREEN_TOP_MARGIN + ((available_height - total_height) / 2) + (line_height / 2);
  char buf[50];

  // Frequency and Spreading Factor
  sprintf(buf, "%.3f MHz  SF%d", _node_prefs->freq, _node_prefs->sf);
  display.drawTextCentered(display.width() / 2, y, buf);

  // Bandwidth and Coding Rate
  y += line_spacing;
  sprintf(buf, "BW%.1f  CR%d", _node_prefs->bw, _node_prefs->cr);
  display.drawTextCentered(display.width() / 2, y, buf);

  // TX Power and Noise Floor
  y += line_spacing;
  sprintf(buf, "TX%ddBm  N%ddB", _node_prefs->tx_power_dbm, radio_driver.getNoiseFloor());
  display.drawTextCentered(display.width() / 2, y, buf);

  // RX and TX packet counts
  y += line_spacing;
  sprintf(buf, "RX %lu/%lu  TX %lu/%lu",
          the_mesh.getNumRecvFlood(), the_mesh.getNumRecvDirect(),
          the_mesh.getNumSentFlood(), the_mesh.getNumSentDirect());
  display.drawTextCentered(display.width() / 2, y, buf);

  // Footer removed for cleaner UI

  return 30000; // Refresh every 30 seconds
}

bool RadioStatsScreen::handleInput(char c) {
  if (c == KEY_CANCEL || c == KEY_LEFT) {
    // Back to reports
    _task->gotoReportsScreen();
    return true;
  }

  return false;
}
