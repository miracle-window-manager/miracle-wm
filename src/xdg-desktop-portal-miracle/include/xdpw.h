#ifndef XDPW_H
#define XDPW_H

#include <wayland-client.h>
#ifdef HAVE_LIBSYSTEMD
#include <systemd/sd-bus.h>
#elif HAVE_LIBELOGIND
#include <elogind/sd-bus.h>
#elif HAVE_BASU
#include <basu/sd-bus.h>
#endif

#include "screencast_common.h"
#include "screenshot_common.h"
#include "config.h"

struct xdpw_state {
	struct wl_list xdpw_sessions;
	sd_bus *bus;
	struct wl_display *wl_display;
	struct pw_loop *pw_loop;
	struct xdpw_screencast_context screencast;
	uint32_t screencast_source_types; // bitfield of enum source_types
	uint32_t screencast_cursor_modes; // bitfield of enum cursor_modes
	uint32_t screencast_version;
	uint32_t screenshot_version;
	struct xdpw_config *config;
	int timer_poll_fd;
	struct wl_list timers;
	struct xdpw_timer *next_timer;
};

struct xdpw_request {
	sd_bus_slot *slot;
};

struct xdpw_session {
	struct wl_list link;
	sd_bus_slot *slot;
	char *session_handle;
	struct xdpw_screencast_session_data screencast_data;
};

typedef void (*xdpw_event_loop_timer_func_t)(void *data);

struct xdpw_timer {
	struct xdpw_state *state;
	xdpw_event_loop_timer_func_t func;
	void *user_data;
	struct timespec at;
	struct wl_list link; // xdpw_state::timers
};

enum {
	PORTAL_RESPONSE_SUCCESS = 0,
	PORTAL_RESPONSE_CANCELLED = 1,
	PORTAL_RESPONSE_ENDED = 2
};

int xdpw_screenshot_init(struct xdpw_state *state);
int xdpw_screencast_init(struct xdpw_state *state);

struct xdpw_request *xdpw_request_create(sd_bus *bus, const char *object_path);
void xdpw_request_destroy(struct xdpw_request *req);

struct xdpw_session *xdpw_session_create(struct xdpw_state *state, sd_bus *bus, char *object_path);
void xdpw_session_destroy(struct xdpw_session *req);

struct xdpw_timer *xdpw_add_timer(struct xdpw_state *state,
	uint64_t delay_ns, xdpw_event_loop_timer_func_t func, void *data);

void xdpw_destroy_timer(struct xdpw_timer *timer);

#endif
