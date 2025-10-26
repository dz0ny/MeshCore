# Claude Code Development Guidelines

This document contains guidelines and patterns for developing the MeshCore companion radio firmware.

## UI Development Best Practices

### Use Macros for Repetitive UI Code

When implementing UI screens with repetitive patterns (like selection indicators, borders, or common rendering logic), **always use C preprocessor macros** to avoid code duplication and maintain consistency.

#### Example: Selection Border Macro

All UI screens that show selectable items use a **thin 1-pixel dotted border** around the selected item. Instead of duplicating the border drawing code in each screen, we use a macro:

```cpp
// Macro to draw thin dotted border around selected item
#define DRAW_SELECTION_BORDER(is_selected, y_pos, spacing) \
  if (is_selected) { \
    display.setColor(DisplayDriver::LIGHT); \
    int border_x = 1, border_y = (y_pos) - 2, border_w = display.width() - 2, border_h = (spacing); \
    /* Top border (dotted) */ \
    for (int x = border_x; x < border_x + border_w; x += 3) { \
      display.fillRect(x, border_y, 1, 1); \
    } \
    /* Bottom border (dotted) */ \
    for (int x = border_x; x < border_x + border_w; x += 3) { \
      display.fillRect(x, border_y + border_h - 1, 1, 1); \
    } \
    /* Left border (dotted) */ \
    for (int y = border_y; y < border_y + border_h; y += 3) { \
      display.fillRect(border_x, y, 1, 1); \
    } \
    /* Right border (dotted) */ \
    for (int y = border_y; y < border_y + border_h; y += 3) { \
      display.fillRect(border_x + border_w - 1, y, 1, 1); \
    } \
    display.setColor(DisplayDriver::GREEN); \
  }
```

**Usage:**
```cpp
// In MessagesScreen.cpp
DRAW_SELECTION_BORDER(is_selected, y, line_height);

// In NearbyScreen.cpp
DRAW_SELECTION_BORDER(item_idx == _selected_index, y, line_spacing);

// In ReportsScreen.cpp
DRAW_SELECTION_BORDER(_selected_item == 0, y, line_spacing);
```

#### Benefits of Using Macros

1. **Consistency**: All screens use identical selection visual style
2. **Maintainability**: Update the border style in one place, affects all screens
3. **Readability**: Single line instead of 10+ lines of drawing code
4. **Performance**: Macros are expanded at compile time (no function call overhead)

#### When to Use Macros vs Helper Functions

**Use Macros when:**
- The code needs to access local variables (like `y`, `display`, `line_spacing`)
- You want zero runtime overhead
- The logic is simple and self-contained
- Type safety isn't a concern

**Use Helper Functions when:**
- The logic is complex and needs debugging
- You need type checking
- The code doesn't depend heavily on local context
- You want better error messages

#### Example: Helper Function Pattern

For more complex rendering logic, combine macros with helper functions:

```cpp
// Helper function to draw a setting item
static void drawSetting(DisplayDriver& display, int& y, int line_spacing,
                       bool is_selected, const char* text) {
  DRAW_SELECTION_BORDER(is_selected, y, line_spacing);
  display.drawTextLeftAlign(5, y, text);
  y += line_spacing;
}
```

This combines the benefits of both approaches:
- Macro handles the border drawing
- Function handles parameter validation and code organization
- The function modifies `y` by reference to advance the cursor

**IMPORTANT**: When using helper functions that increment `y`, DO NOT manually increment `y` again after calling the function. This will create gaps in the UI.

```cpp
// ❌ WRONG - creates gaps
drawSetting(display, y, line_spacing, is_selected, buf);
y += line_spacing;  // DON'T DO THIS - drawSetting already increments y!

// ✅ CORRECT
drawSetting(display, y, line_spacing, is_selected, buf);
// y is already advanced by drawSetting
```

### Screen Layout Guidelines

#### Screen Margins

Use consistent margins for all screens:

```cpp
static constexpr int SCREEN_TOP_MARGIN = 20;    // Space for header
static constexpr int SCREEN_BOTTOM_MARGIN = 5;  // Minimal bottom margin
```

**Important:** Always use `SCREEN_BOTTOM_MARGIN = 5` - screens should NOT have footers with help text. The UI should be self-explanatory through consistent interaction patterns.

#### No Footers with Helper Text

❌ **Do NOT add footers like:**
- "Back: Exit | Sel: View"
- "Up/Down:Select Enter:Map"
- "Back: List"

✅ **Instead:**
- Use consistent button mappings across all screens
- Let users learn through experience
- Keep the UI clean and uncluttered
- The dotted border selection indicator is self-explanatory

### Screen Implementation Checklist

When implementing a new screen with selectable items:

1. ✅ Add the `DRAW_SELECTION_BORDER` macro at the top of the .cpp file
2. ✅ Set `SCREEN_BOTTOM_MARGIN = 5` (no footer text!)
3. ✅ Consider a helper function if you have 3+ similar items to render
4. ✅ Use consistent spacing variables (`line_spacing`, `line_height`)
5. ✅ Ensure selection starts at first item (index 0)
6. ✅ Support wraparound navigation (up from top goes to bottom, down from bottom goes to top)
7. ✅ Remove any footer/helper text - keep the UI clean
8. ✅ Update SCREENS.md documentation

### Refactoring Existing Screens

When refactoring screens to use macros:

1. Identify repetitive patterns (borders, headers, footers)
2. Extract the pattern to a macro with clear parameters
3. Use sed/awk for batch replacements if updating many instances
4. Verify compilation after each major change
5. Test on device to ensure visual consistency

---

*Last Updated: 2025-10-20*
*Related: SCREENS.md*
