#ifndef WM0_H
#define WM0_H

#include <stdio.h>
#include <stdbool.h>
#include <xcb/xcb.h>
#include "config.h"

#define LENGTH(a) (sizeof(a) / sizeof(a[0]))

#ifdef DEBUG
#define LOG(...) printf(__VA_ARGS__)
#else
#define LOG(...)
#endif

// Send a request, and then get its result
#define XCB_REQUEST_AND_CHECK(conn, request, ...) \
    xcb_request_check(conn, xcb_ ## request ## _checked(conn, __VA_ARGS__))

// Send a request, and then get its reply
#define XCB_REQUEST_AND_REPLY(conn, request, e, ...) \
    xcb_ ## request ## _reply(conn, xcb_ ## request ## _unchecked(conn, \
        __VA_ARGS__), e)

// State of the mouse pointer
enum {
    NO_GRAB,     // Pointer is not grabbed
    GRAB_MOVE,   // Pointer is grabbed for moving a window
    GRAB_RESIZE  // Pointer is grabbed for resizing a window
};

// State of the WM
struct wm {
    xcb_connection_t *conn;    // Connection to the X server
    xcb_screen_t *screen;      // X11 screen
    struct {
        int mode;              // State of the mouse pointer
        int16_t x, y;          // Previous coordinate of the mouse pointer
    } grab;
    uint32_t border_active;    // Color for the border of active windows
    uint32_t border_inactive;  // Color for the border of inactive windows
};

extern struct wm wm; // State of the WM

#endif // WM0_H
