#include <stdlib.h>
#include "wm0.h"
#include "window.h"

static void start_pointer_grab(int mode, int16_t x, int16_t y);
static void stop_pointer_grab(void);

// MapRequest indicates that a client sent a MapWindow request.
// When this function is called, the window is not mapped, so WM should map it.
void
handle_map_request(xcb_map_request_event_t *ev)
{
    struct window *win;
    xcb_get_window_attributes_reply_t *r;

    LOG("MapRequest on %x\n", ev->window);

    // Windows with override_redirect flag is not handled by WM.
    r = XCB_REQUEST_AND_REPLY(wm.conn, get_window_attributes, NULL, ev->window);
    if (r != NULL) {
        if (!r->override_redirect) {
            win = window_manage(ev->window);
            if (win != NULL) {
                xcb_map_window(wm.conn, win->id);
                window_focus(win);
            }
        }
    }

    // If we fail to manage the window, map it so that the user can access the
    // window even in that case.
    if (r == NULL || (!r->override_redirect && win == NULL))
        xcb_map_window(wm.conn, ev->window);

    free(r);
}

// UnmapNotify indicates that a window was unmapped.
void
handle_unmap_notify(xcb_unmap_notify_event_t *ev)
{
    struct window *win;

    LOG("UnmapNotify on %x\n", ev->window);

    win = window_find(ev->window);
    if (win != NULL)
        window_unmanage(win);
}

// DestroyNotify indicates that a window was destroyed.
void
handle_destroy_notify(xcb_destroy_notify_event_t *ev)
{
    struct window *win;

    LOG("DestroyNotify on %x\n", ev->window);

    win = window_find(ev->window);
    if (win != NULL)
        window_unmanage(win);
}

// ConfigureRequest indicates that a client sent a ConfigureWindow request.
// When this function is called, the window is not configured, so WM should
// configure it.
void
handle_configure_request(xcb_configure_request_event_t *ev)
{
    uint32_t values[6];
    int i = 0;

    LOG("ConfigureRequest on %x\n", ev->window);

    // Configure the window as requested, except for border width.
    // values must be in the same order as XCB_CONFIG_* are defined.
    if (ev->value_mask & XCB_CONFIG_WINDOW_X)
        values[i++] = ev->x;
    if (ev->value_mask & XCB_CONFIG_WINDOW_Y)
        values[i++] = ev->y;
    if (ev->value_mask & XCB_CONFIG_WINDOW_WIDTH)
        values[i++] = ev->width;
    if (ev->value_mask & XCB_CONFIG_WINDOW_HEIGHT)
        values[i++] = ev->height;
    if (ev->value_mask & XCB_CONFIG_WINDOW_SIBLING)
        values[i++] = ev->sibling;
    if (ev->value_mask & XCB_CONFIG_WINDOW_STACK_MODE)
        values[i++] = ev->stack_mode;
    xcb_configure_window(wm.conn, ev->window, ev->value_mask, values);
}

static void
start_pointer_grab(int mode, int16_t x, int16_t y)
{
    xcb_grab_pointer_reply_t *r;

    wm.grab.mode = mode;
    wm.grab.x = x;
    wm.grab.y = y;

    r = XCB_REQUEST_AND_REPLY(wm.conn, grab_pointer, NULL, false,
        wm.screen->root, XCB_EVENT_MASK_BUTTON_RELEASE |
        XCB_EVENT_MASK_POINTER_MOTION, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC,
        wm.screen->root, XCB_NONE, XCB_CURRENT_TIME);
    if (r == NULL)
        wm.grab.mode = NO_GRAB;
    free(r);
}

static void
stop_pointer_grab()
{
    xcb_ungrab_pointer(wm.conn, XCB_CURRENT_TIME);
    wm.grab.mode = NO_GRAB;
}

// ButtonPress indicates that the mouse button was pressed.
void
handle_button_press(xcb_button_press_event_t *ev)
{
    struct window *win;

    LOG("ButtonPress on %x, modifier=%x, button=%x\n", ev->event, ev->state,
        ev->detail);

    win = window_find(ev->event);
    if (win != NULL) {
        window_raise(win);
        window_focus(win);

        if (ev->state & MODKEY_MASK) {
            int mode;

            switch (ev->detail) {
            case BUTTON_MOVE:
                mode = GRAB_MOVE;
                break;
            case BUTTON_RESIZE:
                mode = GRAB_RESIZE;
                break;
            case BUTTON_CLOSE:
                mode = NO_GRAB;
                window_close(win);
            }
            if (mode != NO_GRAB)
                start_pointer_grab(mode, ev->root_x, ev->root_y);
        }
        xcb_allow_events(wm.conn, XCB_ALLOW_REPLAY_POINTER, XCB_CURRENT_TIME);
    }
}

// ButtonRelease indicates that the mouse button was released.
void
handle_button_release(xcb_button_release_event_t *ev)
{
    LOG("ButtonRelease on %x\n", ev->event);

    if (wm.grab.mode != NO_GRAB)
        stop_pointer_grab();
}

// MotionNotify indicates that the mouse pointer was moved.
void
handle_motion_notify(xcb_motion_notify_event_t *ev)
{
    if (wm.grab.mode != NO_GRAB) {
        struct window *win = window_get_current();
        int16_t dx, dy;

        dx = ev->root_x - wm.grab.x;
        dy = ev->root_y - wm.grab.y;

        if (wm.grab.mode == GRAB_MOVE)
            window_move(win, win->x + dx, win->y + dy);
        else
            window_resize(win, win->w + dx, win->h + dy);

        wm.grab.x = ev->root_x;
        wm.grab.y = ev->root_y;
    }
}
