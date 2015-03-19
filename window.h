#ifndef WM0_WINDOW_H
#define WM0_WINDOW_H

#include <xcb/xcb.h>
#include "queue.h"

// This structure represents a window managed by the WM.
// All windows are added to the TAILQ when it is mapped, and removed when it
// is unmapped.
struct window {
    TAILQ_ENTRY(window) link;  // link for the window list
    xcb_window_t id;           // XID of the window
    int16_t x, y;              // Coordinate of the window (relative to parent)
    uint16_t w, h;             // Width and height of the window
};

void window_init(void);
struct window *window_get_current(void);
struct window *window_find(xcb_window_t id);
struct window *window_manage(xcb_window_t id);
void window_unmanage(struct window *win);
void window_unmanage_all(void);
void window_map(struct window *win);
void window_move(struct window *win, int16_t x, int16_t y);
void window_resize(struct window *win, uint16_t w, uint16_t h);
void window_raise(struct window *win);
void window_focus(struct window *win);
void window_close(struct window *win);

#endif // WM0_WINDOW_H
