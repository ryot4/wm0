// wm0 - A small X11 window manager with libxcb.

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>    // for xcb_aux_*
#include <xcb/xcb_event.h>  // for xcb_event_* and XCB_EVENT_RESPONSE_TYPE
#include "wm0.h"
#include "window.h"

struct wm wm;  // Global state of the WM

// Event handlers
void handle_map_request(xcb_map_request_event_t *ev);
void handle_unmap_notify(xcb_unmap_notify_event_t *ev);
void handle_destroy_notify(xcb_destroy_notify_event_t *ev);
void handle_configure_request(xcb_configure_request_event_t *ev);
void handle_button_press(xcb_button_press_event_t *ev);
void handle_button_release(xcb_button_release_event_t *ev);
void handle_motion_notify(xcb_motion_notify_event_t *ev);

static uint32_t alloc_color(char *rgb_string);
static void init(void);
static void scan(void);
static void run(void);
static void cleanup(void);

// Get pixel value for given RGB string.
static uint32_t
alloc_color(char *rgb_string)
{
    xcb_alloc_color_reply_t *reply;
    uint16_t r, g, b;
    uint32_t pixel;

    xcb_aux_parse_color(rgb_string, &r, &g, &b);
    reply = XCB_REQUEST_AND_REPLY(wm.conn, alloc_color, NULL,
        wm.screen->default_colormap, r, g, b);
    pixel = (reply != NULL) ? reply->pixel : 0;
    free(reply);
    return pixel;
}

// Initialize everything.
static void
init(void)
{
    int screen_num;
    xcb_generic_error_t *e;
    uint32_t event_mask = XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY |
        XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT;

    wm.conn = xcb_connect(NULL, &screen_num);
    if (xcb_connection_has_error(wm.conn)) {
        fputs("cannot open display\n", stderr);
        exit(1);
    }
    wm.screen = xcb_aux_get_screen(wm.conn, screen_num);

    e = XCB_REQUEST_AND_CHECK(wm.conn, change_window_attributes,
        wm.screen->root, XCB_CW_EVENT_MASK, &event_mask);
    if (e != NULL) {
        fputs("another window manager is running\n", stderr);
        exit(1);
    }

    wm.grab.mode = NO_GRAB;
    wm.border_active = alloc_color(COLOR_ACTIVE);
    wm.border_inactive = alloc_color(COLOR_INACTIVE);

    window_init();
}

// Scan existing windows and manage them.
static void
scan(void)
{
    xcb_query_tree_reply_t *tree;
    xcb_window_t *children;
    int n;
    struct window *win = NULL;

    tree = XCB_REQUEST_AND_REPLY(wm.conn, query_tree, NULL, wm.screen->root);
    children = xcb_query_tree_children(tree);
    n = xcb_query_tree_children_length(tree);
    for (int i = 0; i < n; ++i) {
        xcb_get_window_attributes_reply_t *r;

        r = XCB_REQUEST_AND_REPLY(wm.conn, get_window_attributes, NULL,
            children[i]);
        if (!r->override_redirect && r->map_state == XCB_MAP_STATE_VIEWABLE)
            win = window_manage(children[i]);
        free(r);
    }
    free(tree);
    window_focus(win);
}

// Process events.
#define HANDLE_EVENT(type, handler) case type: handler((void *)event); break
static void
run(void)
{
    xcb_generic_event_t *event;

    while ((event = xcb_wait_for_event(wm.conn)) != NULL) {
        if (event->response_type == 0) {
            xcb_generic_error_t *e = (xcb_generic_error_t *)event;

            if (e->error_code != XCB_WINDOW) {
                fprintf(stderr, "X protocol error: request=%s, error=%s\n",
                    xcb_event_get_request_label(e->major_code),
                    xcb_event_get_error_label(e->error_code));
            }
        } else {
            switch (XCB_EVENT_RESPONSE_TYPE(event)) {
                HANDLE_EVENT(XCB_MAP_REQUEST, handle_map_request);
                HANDLE_EVENT(XCB_UNMAP_NOTIFY, handle_unmap_notify);
                HANDLE_EVENT(XCB_DESTROY_NOTIFY, handle_destroy_notify);
                HANDLE_EVENT(XCB_CONFIGURE_REQUEST, handle_configure_request);
                HANDLE_EVENT(XCB_BUTTON_PRESS, handle_button_press);
                HANDLE_EVENT(XCB_BUTTON_RELEASE, handle_button_release);
                HANDLE_EVENT(XCB_MOTION_NOTIFY, handle_motion_notify);
            }
            xcb_flush(wm.conn);
        }
        free(event);
    }
}
#undef HANDLE_EVENT

// Cleanup everything.
static void
cleanup(void)
{
    window_unmanage_all();
    xcb_disconnect(wm.conn);
}

int
main(void)
{
    init();
    scan();
    run();
    cleanup();
    return 0;
}
