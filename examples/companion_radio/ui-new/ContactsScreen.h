#pragma once

#include <helpers/ui/UIScreen.h>
#include <helpers/ui/DisplayDriver.h>
#include <MeshCore.h>
#include "../MyMesh.h"

class UITask;
class SensorManager;
struct NodePrefs;

#ifndef UI_RECENT_LIST_SIZE
  #define UI_RECENT_LIST_SIZE 4
#endif

class ContactsScreen : public UIScreen {
public:
  ContactsScreen(UITask* task, mesh::RTCClock* rtc, SensorManager* sensors, NodePrefs* node_prefs);

  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
  void poll() override {}
  void reset();

private:
  UITask* _task;
  mesh::RTCClock* _rtc;
  SensorManager* _sensors;
  NodePrefs* _node_prefs;
  AdvertPath recent[UI_RECENT_LIST_SIZE];
  int _selected_index;
};
