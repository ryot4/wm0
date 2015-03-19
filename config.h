#ifndef WM0_CONFIG_H
#define WM0_CONFIG_H

// Mouse button assignment (1 = left, 2 = middle, 3 = right)
#define BUTTON_MOVE    1
#define BUTTON_RESIZE  3
#define BUTTON_CLOSE   2

// Modifier key
#define MODKEY_MASK XCB_MOD_MASK_1

// Color of the border of windows
#define COLOR_ACTIVE   "#0000FF"  // for active (focused) window
#define COLOR_INACTIVE "#202020"  // for inactive (not focused) window

#endif // WM0_CONFIG_H
