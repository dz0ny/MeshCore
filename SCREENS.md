# MeshCore Companion Radio - Screen Documentation

This document provides detailed information about all UI screens in the MeshCore companion radio firmware.

## Screen Navigation Overview

The UI uses a screen-based architecture where each screen implements the `UIScreen` interface:
- `render(DisplayDriver& display)` - Draws the screen
- `handleInput(char c)` - Processes button input
- `poll()` - Called periodically for updates

### Button Mapping

- **Back Button** (PIN_BUTTON1): Cancel/Exit current screen
- **Joystick Up** (PIN_BUTTON4): Navigate up / Previous
- **Joystick Down** (PIN_BUTTON5): Navigate down / Next
- **Joystick Left** (PIN_BUTTON3): Navigate left
- **Joystick Right** (PIN_BUTTON2): Navigate right
- **Joystick Press** (PIN_BUTTON6): Select/Enter/Confirm

### Key Codes
- `KEY_UP` (0xB5) - Joystick up
- `KEY_DOWN` (0xB6) - Joystick down
- `KEY_LEFT` (0xB4) - Joystick left
- `KEY_RIGHT` (0xB7) - Joystick right
- `KEY_ENTER` (13) - Joystick press
- `KEY_CANCEL` (27) - Back button

---

## Home Screen

**File**: `examples/companion_radio/ui-new/UITask.cpp`

The home screen is a carousel of multiple pages that can be navigated with left/right buttons.

### Pages (HomePage enum):

#### 1. FIRST Page
- Shows welcome screen with device information
- Displays battery status
- Shows mesh network status

#### 2. SETTINGS Page
- Quick link to settings screen
- Press Enter to access full settings menu

#### 3. MESSAGES Page
- Shows message count badge
- Displays "Messages" label
- Press Enter to view messages
- When new message arrives, home screen jumps to this page automatically

#### 4. NEARBY Page
- Shows count of nearby nodes
- Displays newest node name
- Press Enter/Select to open full Nearby screen

#### 5. ADVERT Page
- Shows current node's advertisement status
- Displays what information is being broadcast

#### 6. SENSORS Page
- Shows all available sensor readings
- Temperature, humidity, pressure
- Battery voltage and percentage
- Updates every 5 seconds

#### 7. SHUTDOWN Page
- Press Enter for options menu
- Can shutdown or restart device

### Navigation:
- **Left/Right**: Cycle through pages
- **Enter/Select**: Enter selected page (opens full screen for Settings, Messages, Nearby)
- **Long Press (varies by page)**: Additional actions

### UI Navigation Rules

**IMPORTANT**: These rules ensure consistent navigation behavior across all home screen pages.

1. **Page Navigation (LEFT/RIGHT) Must Always Work**
   - `KEY_LEFT` and `KEY_RIGHT` MUST cycle through HomePage carousel pages
   - These keys should NOT be intercepted by page-specific handlers
   - Exception: Only when in a sub-view within a page (e.g., NEARBY map detail view)

2. **Page Entry Uses SELECT/ENTER Only**
   - To enter a page's full screen view (e.g., SETTINGS → SettingsScreen, MESSAGES → MessagesScreen)
   - ONLY use `KEY_ENTER` or `KEY_SELECT`
   - NEVER use `KEY_RIGHT` for page entry (it conflicts with navigation)

3. **Page-Specific Input Handling Order**
   ```cpp
   bool handleInput(char c) override {
     // 1. Handle page-specific actions FIRST (ENTER/SELECT only)
     if (_page == HomePage::SETTINGS) {
       if (c == KEY_ENTER || c == KEY_SELECT) {
         _task->gotoSettingsScreen();
         return true;
       }
     }

     // 2. Handle sub-view navigation (if applicable)
     if (_page == HomePage::NEARBY && selected_advert_index >= 0) {
       if (c == KEY_LEFT) {  // Back from sub-view
         selected_advert_index = -1;
         return true;
       }
     }

     // 3. Handle global page navigation LAST (LEFT/RIGHT)
     if (c == KEY_LEFT || c == KEY_PREV) {
       _page = (_page + HomePage::Count - 1) % HomePage::Count;
       return true;
     }
     if (c == KEY_RIGHT || c == KEY_NEXT) {
       _page = (_page + 1) % HomePage::Count;
       return true;
     }
   }
   ```

4. **Common Mistake to Avoid**
   - ❌ WRONG: `if (c == KEY_ENTER || c == KEY_SELECT || c == KEY_RIGHT) { gotoScreen(); }`
   - ✅ CORRECT: `if (c == KEY_ENTER || c == KEY_SELECT) { gotoScreen(); }`
   - Rationale: RIGHT should navigate to next page, not enter current page

5. **Consistency Across Pages**
   - MESSAGES page: Shows preview, ENTER/SELECT to open full MessagesScreen
   - SETTINGS page: Shows preview, ENTER/SELECT to open full SettingsScreen
   - NEARBY page: Shows preview, ENTER/SELECT to open full NearbyScreen
   - GPS page: Shows status, ENTER to toggle
   - All follow the same pattern: RIGHT = next page, ENTER/SELECT = activate/enter

**Code Location**: `examples/companion_radio/ui-new/UITask.cpp:765-860` (HomeScreen::handleInput)

---

## Messages Screen

**Files**:
- `examples/companion_radio/ui-new/MessagesScreen.h`
- `examples/companion_radio/ui-new/MessagesScreen.cpp`

Nokia SMS-style message interface for viewing stored mesh messages.

### Views:

#### List View (Default)
- Shows all messages in scrollable list
- Each entry shows:
  - Unread indicator (`*` for unread)
  - Sender name
  - Message preview (first ~30 chars)
  - Message timestamp
- Empty state: "No messages yet"
- Header: "Messages (X)" where X is message count
- Footer: "Back: Exit | Sel: View"

**Controls**:
- **Up/Down**: Scroll through message list
- **Enter**: View selected message
- **Back**: Return to home screen

#### Message View
Displays full message content when a message is selected.

- Shows sender name at top
- Message text with word wrapping
- Scrollable for long messages
- Footer: "Back: List | Menu: Options"

**Controls**:
- **Up/Down**: Scroll message content
- **Back**: Return to message list
- **Enter**: Open message menu

#### Message Menu
Options menu for current message.

- **Delete**: Remove message from storage
- **Mark Unread**: Mark message as unread
- **Cancel**: Return to message view

**Controls**:
- **Up/Down**: Navigate menu options
- **Enter**: Select option
- **Back**: Cancel and return to message

### Features:
- Persistent storage using MessageStore
- Automatic message pagination (10 messages per page)
- Unread message tracking
- Word-wrapped text display
- Context-sensitive help text in footer

---

## Nearby Screen

**Files**:
- `examples/companion_radio/ui-new/NearbyScreen.h`
- `examples/companion_radio/ui-new/NearbyScreen.cpp`

Displays nearby mesh nodes with distance and time information, with interactive map view.

### Views:

#### List View (Default)
- Shows all nearby mesh nodes in scrollable list
- Each entry shows:
  - Node name
  - Time since last heard (e.g., "2m", "15s", "3h")
  - Distance if GPS available (e.g., "150m", "2.3km")
  - Selection indicator (">") for currently selected node
- Empty state: "No nodes nearby"
- Header: "Nearby Nodes"
- Footer: "Up/Down:Select Enter:Map" or "Back: Home"

**Controls**:
- **Up/Down**: Select node in list
- **Enter/Select**: View map for selected node
- **Back/Left**: Return to home screen

#### Map View
Displays interactive map when a node is selected.

- Shows both user position (solid square) and selected node (hollow square)
- Dotted line connecting the two positions
- Auto-zooms to fit both points
- Node name shown at top
- Distance shown at bottom
- Requires GPS fix on both user and contact node
- Footer: "Back: List"

**Controls**:
- **Back/Left**: Return to list view

### Features:
- Real-time distance calculation using Haversine formula
- GPS location data from contact info
- Time tracking for last heard
- Automatic map centering and zoom
- UTF-8 name support with character translation
- Empty state handling

### Error States:
- "No GPS fix" - User has no GPS lock
- "No GPS data for this node" - Selected node has no GPS coordinates
- "No nodes nearby" - No recent advertisements received

---

## Settings Screen

**Files**:
- `examples/companion_radio/ui-new/SettingsScreen.h`
- `examples/companion_radio/ui-new/SettingsScreen.cpp`

Comprehensive settings menu organized by category.

### Settings Categories:

#### Connectivity
1. **Bluetooth** (Toggle)
   - Enable/disable Bluetooth LE connectivity
   - Allows pairing with mobile apps

2. **Show Radio Stats** (Action)
   - Opens Radio Stats screen
   - View radio configuration and packet statistics

#### Sound
3. **Buzzer** (Toggle)
   - Enable/disable all buzzer sounds
   - Requires `PIN_BUZZER` to be defined

4. **Key Press Buzzer** (Toggle)
   - Enable/disable beep on button press
   - Requires `PIN_BUZZER` to be defined

#### GPS - *if GPS hardware available*
5. **GPS** (Toggle)
   - Enable/disable GPS module
   - Affects power consumption

**GPS Info** (Display only when GPS is ON):
- Status: Shows FIX or SEARCH state
- Lat: Latitude in decimal degrees
- Lon: Longitude in decimal degrees
- Alt: Altitude in meters
- Acc: Accuracy in meters

#### Location Advertisement - *if GPS hardware available*
6. **Broadcast Location** (Toggle)
   - Enable/disable GPS location advertising
   - Broadcasts GPS position to mesh network

7. **Movement Threshold** (Value: 10-500m)
   - Minimum distance moved before broadcasting update
   - Options: 10m, 50m, 100m, 200m, 500m
   - Reduces unnecessary broadcasts

8. **Update Frequency** (Value: 30-300 seconds)
   - Maximum time between location broadcasts
   - Options: 30s, 60s, 120s, 300s
   - Even if not moved

9. **Guaranteed Interval** (Value: 1-15 minutes)
   - Minimum time between location checks
   - Options: 1m, 5m, 15m
   - Affects power consumption

10. **Required Accuracy** (Value: 5-100m)
    - Required GPS accuracy before broadcasting
    - Options: 5m, 10m, 20m, 50m, 100m
    - Prevents broadcasting inaccurate positions

#### Privacy
12. **Telemetry Share** (Cycle: DENY / FLAGS / ALL)
    - DENY: No telemetry shared
    - FLAGS: Share basic status flags
    - ALL: Share all sensor data

13. **Advertise Location** (Toggle)
    - Master switch for location sharing in advertisements
    - Independent of GPS Location Broadcast settings

#### Misc
14. **Show Key Presses** (Action)
    - Opens Debug Key screen
    - View encryption keys and node IDs

### Display Format:
```
Settings
--------------------
 * Connectivity

 > Bluetooth: ON
   GPS: ON
   Show Radio Stats

 * Sound

   Buzzer: ON
   Key Press Buzzer: ON

 * GPS

 > GPS: ON
   Status: FIX
   Lat: 37.77493
   Lon: -122.41942
   Alt: 15.2m
   Acc: 12m

 * Location Advert

   Broadcast Location: ON
   Movement Threshold: 100m
   Update Frequency: 60s
   Guaranteed Interval: 5m
   Required Accuracy: 20m

 * Privacy

   Telemetry Share: FLAGS
   Advertise Location: ON

 * Misc

   Show Key Presses
```

### Navigation:
- **Up/Down**: Navigate through settings
- **Enter**: Toggle or adjust selected setting
- **Left/Right**: Adjust numeric values (when applicable)
- **Back**: Return to home screen

### Features:
- Auto-scrolling for long lists
- Visual indicators for current selection (">")
- Section headers for organization
- Real-time value updates
- Settings saved to persistent storage via `the_mesh.savePrefs()`
- Toast notifications for setting changes

---

## Radio Stats Screen

**Files**:
- `examples/companion_radio/ui-new/RadioStatsScreen.h`
- `examples/companion_radio/ui-new/RadioStatsScreen.cpp`

Displays detailed radio configuration and packet statistics.

### Display Information:

#### Radio Configuration
- **Frequency**: Current operating frequency in MHz (e.g., "915.000 MHz")
- **Spreading Factor**: LoRa SF parameter (e.g., "SF7")
- **Bandwidth**: Channel bandwidth in kHz (e.g., "BW125.0")
- **Coding Rate**: Error correction coding rate (e.g., "CR5")
- **TX Power**: Transmit power in dBm (e.g., "TX20dBm")
- **Noise Floor**: Current noise floor in dB (e.g., "N-120dB")

#### Packet Statistics
- **RX Flood/Direct**: Received packets (flood and direct)
- **TX Flood/Direct**: Transmitted packets (flood and direct)

Example display:
```
Radio Stats
--------------------

915.000 MHz  SF7
BW125.0  CR5
TX20dBm  N-120dB
RX 145/23  TX 98/12

Back: Exit
```

### Navigation:
- **Back/Left**: Return to Settings screen

### Features:
- Real-time statistics (updates every second)
- Centered layout for readability
- Consistent header and footer styling
- Noise floor monitoring

---

## Debug Key Screen

**Files**:
- `examples/companion_radio/ui-new/DebugKeyScreen.h`
- `examples/companion_radio/ui-new/DebugKeyScreen.cpp`

Developer screen showing cryptographic keys and node identifiers.

### Display Information:

#### Network Keys
- **Session Key**: Current mesh session encryption key (hex)
- **Identity Public Key**: Node's Ed25519 public key (hex)

#### Node Identifiers
- **Short ID**: 4-byte node identifier
- **Long ID**: 8-byte extended identifier

### Display Format:
```
Debug Keys
--------------------

Session Key:
A1B2C3D4E5F6...

ID Public Key:
9F8E7D6C5B4A...

Short: 1A2B3C4D
Long: 1A2B3C4D5E6F7890

Back: Exit
```

### Navigation:
- **Back/Left**: Return to home screen

### Security Note:
This screen is for debugging only. In production, consider disabling or protecting access to this screen.

---

## Screen Transitions

### Common Transition Paths:

```
Home Screen
  ├─> Messages Screen (Enter/Select on MESSAGES page)
  │     ├─> Message View (Select message)
  │     │     └─> Message Menu (Enter in message view)
  │     │           └─> Back to Message View or List
  │     └─> Back to Home
  │
  ├─> Nearby Screen (Enter/Select on NEARBY page)
  │     ├─> Map View (Select node + Enter)
  │     │     └─> Back to Node List
  │     └─> Back to Home
  │
  ├─> Settings Screen (Enter/Select on SETTINGS page)
  │     ├─> Radio Stats Screen (Select "Show Radio Stats")
  │     │     └─> Back to Settings
  │     ├─> Debug Key Screen (Select "Show Key Presses")
  │     │     └─> Back to Home
  │     └─> Back to Home
  │
  └─> GPS, SENSORS, ADVERT, SHUTDOWN pages
        └─> Back to Home
```

### Automatic Transitions:

1. **New Message Received**:
   - Home screen automatically jumps to MESSAGES page
   - Does NOT automatically enter Messages Screen
   - User must press Enter to view messages

2. **Display Timeout**:
   - Screen turns off after inactivity period
   - Press any button to wake

3. **Low Battery**:
   - May automatically navigate to SHUTDOWN page

---

## Display Specifications

### Layout Constants

- **SCREEN_TOP_MARGIN**: 22 pixels
  - Accounts for header and separator line

- **SCREEN_BOTTOM_MARGIN**: 10 pixels
  - Accounts for footer/help text

### Text Sizes
- **Size 0**: Small font (6-7pt) - Used for footer text
- **Size 1**: Normal font (8-9pt) - Used for body text
- **Size 2**: Large font - Used for titles (where supported)

### Colors (DisplayDriver enum)
- **LIGHT**: Primary text color (white on dark, black on light)
- **DARK**: Background color
- **YELLOW**: Accent color for highlights and important info

### Visual Elements

#### Header Pattern
```cpp
display.setTextSize(1);
display.setCursor(0, 0);
display.print("Screen Name");

// Separator line at y=20
for (int dx = 0; dx < display.width(); dx += 3) {
  display.fillRect(dx, 20, 1, 1);
}
```

#### Footer Pattern
```cpp
display.setTextSize(0);
display.setCursor(2, display.height() - 10);
display.print("Back: Exit | Sel: Action");
```

---

## Code Integration

### Adding a New Screen

1. **Create header file** (`NewScreen.h`):
```cpp
#pragma once
#include <helpers/ui/UIScreen.h>
#include <helpers/ui/DisplayDriver.h>

class UITask;
class SensorManager;
class NodePrefs;

class NewScreen : public UIScreen {
public:
  NewScreen(UITask* task, SensorManager* sensors, NodePrefs* node_prefs);

  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
  void poll() override;

private:
  UITask* _task;
  SensorManager* _sensors;
  NodePrefs* _node_prefs;
  // Add state variables
};
```

2. **Create implementation** (`NewScreen.cpp`):
```cpp
#include "NewScreen.h"
#include "UITask.h"

NewScreen::NewScreen(UITask* task, SensorManager* sensors, NodePrefs* node_prefs)
  : _task(task), _sensors(sensors), _node_prefs(node_prefs) {
}

int NewScreen::render(DisplayDriver& display) {
  display.clear();
  display.startFrame();

  // Draw header
  display.setTextSize(1);
  display.setColor(DisplayDriver::LIGHT);
  display.setCursor(0, 0);
  display.print("New Screen");

  // Draw separator
  for (int dx = 0; dx < display.width(); dx += 3) {
    display.fillRect(dx, 20, 1, 1);
  }

  // Draw content
  // ...

  // Draw footer
  display.setTextSize(0);
  display.setCursor(2, display.height() - 10);
  display.print("Back: Exit");

  return 1000; // Refresh interval in ms
}

bool NewScreen::handleInput(char c) {
  if (c == KEY_CANCEL || c == KEY_LEFT) {
    _task->gotoHomeScreen();
    return true;
  }

  // Handle other inputs
  return false;
}

void NewScreen::poll() {
  // Optional periodic updates
}
```

3. **Register in UITask.h**:
```cpp
#include "NewScreen.h"

class UITask : public AbstractUITask {
  // ...
  UIScreen* new_screen;

public:
  void gotoNewScreen() {
    setCurrScreen(new_screen);
  }
};
```

4. **Initialize in UITask.cpp**:
```cpp
void UITask::begin(DisplayDriver* display, SensorManager* sensors, NodePrefs* node_prefs) {
  // ...
  new_screen = new NewScreen(this, sensors, node_prefs);
}
```

5. **Add navigation** from appropriate screens

---

## Memory Management

### Display Buffer
- E-ink displays use full-screen buffers
- Each render clears previous content
- Minimize redraws to reduce flicker

### Message Storage
- Messages stored in binary format via MessageStore
- Stored in internal flash (LittleFS)
- Limited by available flash space
- Consider implementing message expiry

### Settings Storage
- Settings saved via `the_mesh.savePrefs()`
- Persisted in NodePrefs structure
- Stored in internal flash
- Automatic save on setting changes

---

## Performance Considerations

### Refresh Rates
- **Static content**: 30000ms (30 seconds)
- **Dynamic content**: 1000ms (1 second)
- **Critical updates**: 100ms
- **Display wake**: Immediate

### Button Debouncing
- Handled by MomentaryButton class
- Click detection with long-press support
- Double-click and triple-click detection
- Hardware debounce + software filtering

### Power Optimization
- Display auto-off after timeout
- GPS can be disabled in settings
- Sensor polling can be adjusted
- Radio sleep modes utilized

---

## Future Enhancements

### Potential Additions:
1. **Compass Screen**: Show heading and nearby nodes
2. **Route History**: Track and display movement path
3. **Node Directory**: Searchable list of known mesh nodes
4. **Message Compose**: Send messages from device
5. **File Transfer**: Send/receive files over mesh
6. **Photo Gallery**: View received images
7. **Weather Screen**: Display sensor data as weather info
8. **Battery Graph**: Historical battery level chart
9. **Signal Map**: Heat map of signal strength
10. **Settings Profiles**: Quick-switch between configurations

---

## Troubleshooting

### Common Issues:

**Buttons not responding:**
- Verify `begin()` called on all MomentaryButton instances
- Check pin definitions in variant.h
- Verify KEY_* codes match between screens

**Display not updating:**
- Check return value from `render()` - must be > 0
- Verify `display.startFrame()` called before drawing
- E-ink displays need full refresh cycles

**Settings not saving:**
- Ensure `the_mesh.savePrefs()` called after changes
- Check flash space available
- Verify NodePrefs structure matches expectations

**Navigation broken:**
- Check `handleInput()` return values (true = handled)
- Verify `setCurrScreen()` called on transitions
- Ensure curr screen pointer is valid

**Memory issues:**
- Monitor RAM usage (currently 63.4%)
- Limit string buffers in stack
- Use PROGMEM for const strings
- Consider dynamic allocation carefully

---

## Testing Checklist

### Per-Screen Testing:
- [ ] Screen renders without artifacts
- [ ] Header displays correctly
- [ ] Separator line positioned properly
- [ ] Content area scrolls if needed
- [ ] Footer shows correct help text
- [ ] All buttons perform expected actions
- [ ] Back button returns to correct screen
- [ ] Long press actions work
- [ ] Settings persist across reboots
- [ ] Updates reflect in real-time

### Integration Testing:
- [ ] Navigation between all screens works
- [ ] Home screen carousel functions
- [ ] Message notifications trigger correctly
- [ ] Settings changes affect behavior
- [ ] Battery monitoring works
- [ ] GPS lock status accurate
- [ ] Radio stats update
- [ ] Memory usage stable
- [ ] No crashes or hangs
- [ ] Display timeout works

---

## References

### Key Files:
- `src/helpers/ui/UIScreen.h` - Base screen interface
- `src/helpers/ui/DisplayDriver.h` - Display abstraction
- `src/helpers/ui/MomentaryButton.h` - Button handling
- `examples/companion_radio/MessageStore.h` - Message persistence
- `examples/companion_radio/NodePrefs.h` - Settings structure
- `examples/companion_radio/MyMesh.h` - Mesh network interface

### Documentation:
- MeshCore API documentation
- PlatformIO build system
- nRF52840 SDK documentation
- GxEPD2 e-ink library

---

*Last Updated: 2025-10-19*
*Firmware Version: Based on dev branch*
