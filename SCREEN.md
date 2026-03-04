# Companion Radio UI Screen Structure

## Screen Hierarchy

```
SplashScreen (boot)
    ↓ (auto-dismisses after 3 seconds)

HomeScreen (main hub with 6 pages)
    ├─ Page 0: FIRST (connection/BLE PIN status)
    ├─ Page 1: CONTACTS (preview of nearby contacts)
    ├─ Page 2: ADVERT (location advertising)
    ├─ Page 3: MESSAGES (message inbox preview)
    ├─ Page 4: REPORTS (reports menu preview)
    └─ Page 5: SETTINGS (settings preview)

    Navigation:
    - LEFT/RIGHT or PREV/NEXT: Cycle through pages
    - ENTER: Open corresponding full screen
    - BACK: Navigate to ADVERT page (from any page)

From HomeScreen, ENTER opens full screens:

    ├─→ MessagesScreen (from MESSAGES page)
    │   ├─ LIST_VIEW (message list - first page)
    │   ├─ MESSAGE_VIEW (reading a message)
    │   └─ MENU_VIEW (delete menu)
    │   Navigation:
    │   - BACK from sub-views: Returns to LIST_VIEW
    │   - BACK from LIST_VIEW: Returns to HomeScreen → ADVERT page
    │
    ├─→ ContactsScreen (from CONTACTS page)
    │   └─ Contact list view
    │   Navigation:
    │   - UP/DOWN: Navigate contacts
    │   - BACK: Returns to HomeScreen → ADVERT page
    │
    ├─→ SettingsScreen (from SETTINGS page)
    │   └─ Scrollable list of settings
    │   Navigation:
    │   - UP/DOWN: Navigate settings
    │   - ENTER: Edit selected setting
    │   - BACK: Returns to HomeScreen → ADVERT page
    │
    └─→ ReportsScreen (from REPORTS page)
        └─ Menu with 3 items:
            - GPS Info
            - Radio Stats
            - Telemetry
        Navigation:
        - UP/DOWN: Select menu item
        - ENTER: Open selected info screen
        - BACK: Returns to HomeScreen → ADVERT page

        From ReportsScreen, ENTER opens info screens:

        ├─→ GPSScreen
        │   Displays GPS position, satellites, accuracy
        │   BACK: Returns to ReportsScreen
        │
        ├─→ RadioStatsScreen
        │   Displays radio statistics and metrics
        │   BACK: Returns to ReportsScreen
        │
        └─→ TelemetryScreen
            Displays sensor telemetry data
            BACK: Returns to ReportsScreen

Additional Screens:

    MsgPreviewScreen (popup overlay)
        Quick message preview popup
        Auto-dismisses or dismissed with any key
```

## Screen Details

### HomeScreen Pages

| Page | Enum Value | Description | ENTER Action |
|------|-----------|-------------|--------------|
| FIRST | 0 | Connection status / BLE PIN display | None (special: any key → ADVERT when connected) |
| CONTACTS | 1 | Preview of recently heard contacts with count | Opens ContactsScreen |
| ADVERT | 2 | Location info and "Send location" action | Sends advertisement packet |
| MESSAGES | 3 | Message inbox with unread count | Opens MessagesScreen |
| REPORTS | 4 | Reports menu preview | Opens ReportsScreen |
| SETTINGS | 5 | Settings menu preview | Opens SettingsScreen |

### Navigation Keys

| Key | Constant | Function |
|-----|----------|----------|
| ESC | KEY_CANCEL (27) | Back button - navigates to ADVERT page on HomeScreen |
| Enter | KEY_ENTER | Select/confirm action |
| Enter | KEY_SELECT | Alternative select key |
| Left | KEY_LEFT | Navigate left/previous page |
| Right | KEY_RIGHT | Navigate right/next page |
| Left | KEY_PREV | Alternative previous |
| Right | KEY_NEXT | Alternative next |
| Up | KEY_UP | Scroll up in lists |
| Down | KEY_DOWN | Scroll down in lists |

### Back Button Behavior Summary

| Screen | Context | Back Button Action |
|--------|---------|-------------------|
| **HomeScreen** | Any page | → ADVERT page (page 2) |
| **MessagesScreen** | MESSAGE_VIEW | → LIST_VIEW |
| **MessagesScreen** | LIST_VIEW | → HomeScreen (ADVERT) |
| **ContactsScreen** | List view | → HomeScreen (ADVERT) |
| **SettingsScreen** | Any setting | → HomeScreen (ADVERT) |
| **ReportsScreen** | Any item | → HomeScreen (ADVERT) |
| **GPSScreen** | (only view) | → ReportsScreen |
| **RadioStatsScreen** | (only view) | → ReportsScreen |
| **TelemetryScreen** | (only view) | → ReportsScreen |

## UITask Navigation Methods

The UITask class provides these navigation methods for screen transitions:

```cpp
// HomeScreen navigation
void gotoHomeScreen()           // Go to HomeScreen (current page)
void gotoAdvertPage()           // Go to HomeScreen ADVERT page
void gotoMessagesHomePage()     // Go to HomeScreen MESSAGES page

// Full screen navigation
void gotoMessagesScreen()       // Open MessagesScreen (LIST_VIEW)
void gotoContactsScreen()       // Open ContactsScreen (List View)
void gotoSettingsScreen()       // Open SettingsScreen
void gotoReportsScreen()        // Open ReportsScreen

// Info screen navigation (from Reports)
void gotoGPSScreen()           // Open GPSScreen
void gotoRadioStatsScreen()    // Open RadioStatsScreen
void gotoTelemetryScreen()     // Open TelemetryScreen
```

## File Locations

All UI screens are located in: `examples/companion_radio/ui-new/`

| Screen | Header File | Implementation File |
|--------|-------------|-------------------|
| UITask (manager) | UITask.h | UITask.cpp |
| SplashScreen | SplashScreen.h | SplashScreen.cpp |
| HomeScreen | HomeScreen.h | HomeScreen.cpp |
| MessagesScreen | MessagesScreen.h | MessagesScreen.cpp |
| ContactsScreen | ContactsScreen.h | ContactsScreen.cpp |
| SettingsScreen | SettingsScreen.h | SettingsScreen.cpp |
| ReportsScreen | ReportsScreen.h | ReportsScreen.cpp |
| GPSScreen | GPSScreen.h | GPSScreen.cpp |
| RadioStatsScreen | RadioStatsScreen.h | RadioStatsScreen.cpp |
| TelemetryScreen | TelemetryScreen.h | TelemetryScreen.cpp |
| MsgPreviewScreen | MsgPreviewScreen.h | MsgPreviewScreen.cpp |

## Base Classes

### UIScreen

Base class for all screens: `src/helpers/ui/UIScreen.h`

Key virtual methods:
- `bool handleInput(char c)` - Process key input
- `int render(DisplayDriver& display)` - Render screen content
- `void poll()` - Called periodically for updates

### UITask

Central screen manager that:
- Manages current screen state
- Routes input to current screen
- Provides navigation methods
- Handles screen transitions
- Manages display updates

## Display Regions

### Common Layout

```
┌─────────────────────────────────┐
│ Node Name          [Battery Icon]│  ← Header (0-14px)
│ ● ● ● ○ ● ●                     │  ← Page indicators (14-18px)
├ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ┤  ← Dotted separator (18px)
│                                 │
│                                 │
│         Content Area            │  ← Screen content
│                                 │
│                                 │
└─────────────────────────────────┘
```

### Constants

```cpp
#define SCREEN_TOP_MARGIN    22  // Top margin for content
#define SCREEN_BOTTOM_MARGIN 0   // Bottom margin
#define RECENT_BUFFER_SIZE   8   // Max items in preview lists
```

## Recent Updates

### 2025-10-28
- **Removed Map View from ContactsScreen**
  - Simplified ContactsScreen to only show contact list
  - Removed map visualization, cached map calculations, and related methods
  - Updated navigation: removed ENTER toggle, only UP/DOWN and BACK

- **Removed SystemStatsScreen**
  - Deleted SystemStatsScreen.h and SystemStatsScreen.cpp
  - Removed from ReportsScreen menu (was item 4)
  - Removed UITask navigation method and screen instance

- **Removed DebugKeyScreen (buttons test screen)**
  - Deleted DebugKeyScreen.h and DebugKeyScreen.cpp
  - Removed from ReportsScreen menu (was item 3)
  - Removed UITask navigation method and screen instance

- **Updated ReportsScreen**
  - Reduced menu items from 5 to 3
  - Current menu: GPS Info, Radio Stats, Telemetry
  - Updated navigation to handle 3 items (0-2)

- **Added back button navigation to ADVERT page on HomeScreen**
  - Back button (KEY_CANCEL) from any HomeScreen page now navigates to ADVERT page
  - Implementation: `HomeScreen.cpp:355-359`
