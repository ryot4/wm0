#include <stdlib.h>
#include "wm0.h"
#include "window.h"

static TAILQ_HEAD(windows, window) windows;  // List of windows
static struct window *current;               // Currently focused window

// Establish a passive grab of the mouse on the given window to receive a
// ButtonPress event when the mouse button is pressed.
static void
grab_buttons(xcb_window_t id)
{
    uint8_t buttons[] = { BUTTON_MOVE, BUTTON_RESIZE, BUTTON_CLOSE };
    uint16_t modifiers[] = { 0, XCB_MOD_MASK_LOCK };

#define GRAB_BUTTON(id, index, modifier) \
    xcb_grab_button(wm.conn, false, id, XCB_EVENT_MASK_BUTTON_PRESS, \
        XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE, index, \
        modifier)

    for (int i = 0; i < LENGTH(buttons); ++i) {
        for (int j = 0; j < LENGTH(modifiers); ++j) {
            GRAB_BUTTON(id, buttons[i], modifiers[j]);
            GRAB_BUTTON(id, buttons[i], MODKEY_MASK | modifiers[j]);
        }
    }

#undef GRAB_BUTTON
}

void
window_init(void)
{
    TAILQ_INIT(&windows);
    current = NULL;
}

struct window *
window_get_current(void)
{
    return current;
}

struct window *
window_find(xcb_window_t id)
{
    struct window *win;

    TAILQ_FOREACH(win, &windows, link) {
        if (win->id == id)
            return win;
    }
    return NULL;
}

struct window *
window_manage(xcb_window_t id)
{
    struct window *win;
    xcb_get_geometry_reply_t *r;

    LOG("manage %x\n", id);

    win = malloc(sizeof(struct window));
    if (win == NULL)
        return NULL;

    r = XCB_REQUEST_AND_REPLY(wm.conn, get_geometry, NULL, id);
    if (r == NULL) {
        free(win);
        return NULL;
    }

    win->id = id;
    win->x = r->x;
    win->y = r->y;
    win->w = r->width;
    win->h = r->height;

    grab_buttons(win->id);

    TAILQ_INSERT_HEAD(&windows, win, link);

    free(r);
    return win;
}

void
window_unmanage(struct window *win)
{
    LOG("unmanage %x\n", win->id);

    if (win == current)
        window_focus(NULL);

    TAILQ_REMOVE(&windows, win, link);
    free(win);
}

void
window_unmanage_all(void)
{
    while (TAILQ_FIRST(&windows) != NULL)
        window_unmanage(TAILQ_FIRST(&windows));
}

void
window_move(struct window *win, int16_t x, int16_t y)
{
    win->x = x;
    win->y = y;
    xcb_configure_window(wm.conn, win->id,
        XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
        (const uint32_t []) { x, y });
}

void
window_resize(struct window *win, uint16_t w, uint16_t h)
{
    win->w = w;
    win->h = h;
    xcb_configure_window(wm.conn, win->id,
        XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
        (const uint32_t []) { w, h });
}

void
window_raise(struct window *win)
{
    xcb_configure_window(wm.conn, win->id,
        XCB_CONFIG_WINDOW_STACK_MODE,
        (const uint32_t []) { XCB_STACK_MODE_ABOVE });
}

void
window_focus(struct window *win)
{
    xcb_window_t to_focus;

    if (win && win == current)
        return;

    if (current != NULL) {
        xcb_change_window_attributes(wm.conn, current->id,
            XCB_CW_BORDER_PIXEL, &wm.border_inactive);
    }

    if (win) {
        xcb_change_window_attributes(wm.conn, win->id, XCB_CW_BORDER_PIXEL,
            &wm.border_active);
        current = win;
        to_focus = win->id;
    } else {
        current = NULL;
        to_focus = wm.screen->root;
    }

    LOG("focus %x\n", to_focus);

    xcb_set_input_focus(wm.conn, XCB_INPUT_FOCUS_PARENT, to_focus,
        XCB_CURRENT_TIME);
}

void
window_close(struct window *win)
{
    LOG("close %x\n", win->id);

    // In fact, this does not "close" the window, but forces the close down of
    // the owner of the window, like xkill(1) does. It's very dangerous.
    // To close the window gracefully, we should send WM_DELETE_WINDOW
    // ClientMessage to the client.
    xcb_kill_client(wm.conn, win->id);
}
