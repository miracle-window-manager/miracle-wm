#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/timerfd.h>
#include <getopt.h>
#include <poll.h>
#include <pipewire/pipewire.h>
#include <spa/utils/result.h>
#include <unistd.h>

#include "xdpw.h"
#include "logger.h"

enum event_loop_fd {
	EVENT_LOOP_DBUS,
	EVENT_LOOP_WAYLAND,
	EVENT_LOOP_PIPEWIRE,
	EVENT_LOOP_TIMER,
};

static const char service_name[] = "org.freedesktop.impl.portal.desktop.wlr";

static int xdpw_usage(FILE *stream, int rc) {
	static const char *usage =
		"Usage: xdg-desktop-portal-wlr [options]\n"
		"\n"
		"    -l, --loglevel=<loglevel>        Select log level (default is ERROR).\n"
		"                                     QUIET, ERROR, WARN, INFO, DEBUG, TRACE\n"
		"    -c, --config=<config file>	      Select config file.\n"
		"                                     (default is $XDG_CONFIG_HOME/xdg-desktop-portal-wlr/config)\n"
		"    -r, --replace                    Replace a running instance.\n"
		"    -h, --help                       Get help (this text).\n"
		"\n";

	fprintf(stream, "%s", usage);
	return rc;
}

static int handle_name_lost(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
	logprint(INFO, "dbus: lost name, closing connection");
	sd_bus_close(sd_bus_message_get_bus(m));
	return 1;
}

int main(int argc, char *argv[]) {
	struct xdpw_config config = {0};
	char *configfile = NULL;
	enum LOGLEVEL loglevel = DEFAULT_LOGLEVEL;
	bool replace = false;

	static const char *shortopts = "l:o:c:f:rh";
	static const struct option longopts[] = {
		{ "loglevel", required_argument, NULL, 'l' },
		{ "config", required_argument, NULL, 'c' },
		{ "replace", no_argument, NULL, 'r' },
		{ "help", no_argument, NULL, 'h' },
		{ NULL, 0, NULL, 0 }
	};

	while (1) {
		int c = getopt_long(argc, argv, shortopts, longopts, NULL);
		if (c < 0) {
			break;
		}

		switch (c) {
		case 'l':
			loglevel = get_loglevel(optarg);
			break;
		case 'c':
			configfile = strdup(optarg);
			break;
		case 'r':
			replace = true;
			break;
		case 'h':
			return xdpw_usage(stdout, EXIT_SUCCESS);
		default:
			return xdpw_usage(stderr, EXIT_FAILURE);
		}
	}

	init_logger(stderr, loglevel);
	init_config(&configfile, &config);
	print_config(DEBUG, &config);

	int ret = 0;

	sd_bus *bus = NULL;
	ret = sd_bus_open_user(&bus);
	if (ret < 0) {
		logprint(ERROR, "dbus: failed to connect to user bus: %s", strerror(-ret));
		return EXIT_FAILURE;
	}
	logprint(DEBUG, "dbus: connected");

	struct wl_display *wl_display = wl_display_connect(NULL);
	if (!wl_display) {
		logprint(ERROR, "wayland: failed to connect to display");
		sd_bus_unref(bus);
		return EXIT_FAILURE;
	}
	logprint(DEBUG, "wlroots: wl_display connected");

	pw_init(NULL, NULL);
	struct pw_loop *pw_loop = pw_loop_new(NULL);
	if (!pw_loop) {
		logprint(ERROR, "pipewire: failed to create loop");
		wl_display_disconnect(wl_display);
		sd_bus_unref(bus);
		return EXIT_FAILURE;
	}
	logprint(DEBUG, "pipewire: pw_loop created");

	struct xdpw_state state = {
		.bus = bus,
		.wl_display = wl_display,
		.pw_loop = pw_loop,
		.screencast_source_types = MONITOR,
		.screencast_cursor_modes = HIDDEN | EMBEDDED,
		.screencast_version = XDP_CAST_PROTO_VER,
		.screenshot_version = XDP_SHOT_PROTO_VER,
		.config = &config,
	};

	wl_list_init(&state.xdpw_sessions);

	ret = xdpw_screenshot_init(&state);
	if (ret < 0) {
		logprint(ERROR, "xdpw: failed to initialize screenshot");
		goto error;
	}

	ret = xdpw_screencast_init(&state);
	if (ret < 0) {
		logprint(ERROR, "xdpw: failed to initialize screencast");
		goto error;
	}

	uint64_t flags = SD_BUS_NAME_ALLOW_REPLACEMENT;
	if (replace) {
		flags |= SD_BUS_NAME_REPLACE_EXISTING;
	}

	ret = sd_bus_request_name(bus, service_name, flags);
	if (ret < 0) {
		logprint(ERROR, "dbus: failed to acquire service name: %s", strerror(-ret));
		goto error;
	}

	const char *unique_name;
	ret = sd_bus_get_unique_name(bus, &unique_name);
	if (ret < 0) {
		logprint(ERROR, "dbus: failed to get unique bus name: %s", strerror(-ret));
		goto error;
	}

	static char match[1024];
	snprintf(match, sizeof(match), "sender='org.freedesktop.DBus',"
		"type='signal',"
		"interface='org.freedesktop.DBus',"
		"member='NameOwnerChanged',"
		"path='/org/freedesktop/DBus',"
		"arg0='%s',"
		"arg1='%s'",
		service_name, unique_name);

	sd_bus_slot *slot;
	ret = sd_bus_add_match(bus, &slot, match, handle_name_lost, NULL);
	if (ret < 0) {
		logprint(ERROR, "dbus: failed to add NameOwnerChanged signal match: %s", strerror(-ret));
		goto error;
	}

	wl_list_init(&state.timers);

	struct pollfd pollfds[] = {
		[EVENT_LOOP_DBUS] = {0}, // Filled in later
		[EVENT_LOOP_WAYLAND] = {
			.fd = wl_display_get_fd(state.wl_display),
			.events = POLLIN,
		},
		[EVENT_LOOP_PIPEWIRE] = {
			.fd = pw_loop_get_fd(state.pw_loop),
			.events = POLLIN,
		},
		[EVENT_LOOP_TIMER] = {
			.fd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC),
			.events = POLLIN,
		}
	};

	state.timer_poll_fd = pollfds[EVENT_LOOP_TIMER].fd;

	while (1) {
		// sd-bus requires that we update FD/events/timeout every time we poll
		pollfds[EVENT_LOOP_DBUS].fd = sd_bus_get_fd(state.bus);
		if (pollfds[EVENT_LOOP_DBUS].fd < 0) {
			logprint(ERROR, "sd_bus_get_fd failed: %s",
				strerror(-pollfds[EVENT_LOOP_DBUS].fd));
			goto error;
		}
		pollfds[EVENT_LOOP_DBUS].events = sd_bus_get_events(state.bus);
		if (pollfds[EVENT_LOOP_DBUS].events < 0) {
			logprint(ERROR, "sd_bus_get_events failed: %s",
				strerror(-pollfds[EVENT_LOOP_DBUS].events));
			goto error;
		}
		uint64_t usec_timeout = 0;
		ret = sd_bus_get_timeout(state.bus, &usec_timeout);
		if (ret < 0) {
			logprint(ERROR, "sd_bus_get_timeout failed: %s", strerror(-ret));
			goto error;
		}
		// Convert timestamp from usec to msec.  Value of -1 indicates no
		// timeout, i.e. poll forever.
		int msec_timeout = usec_timeout == UINT64_MAX ? -1 : (int)((usec_timeout + 999) / 1000);

		ret = poll(pollfds, sizeof(pollfds) / sizeof(pollfds[0]), msec_timeout);
		if (ret < 0) {
			logprint(ERROR, "poll failed: %s", strerror(errno));
			goto error;
		}

		if (pollfds[EVENT_LOOP_DBUS].revents & POLLHUP) {
			logprint(INFO, "event-loop: disconnected from dbus");
			break;
		}
		if (pollfds[EVENT_LOOP_WAYLAND].revents & POLLHUP) {
			logprint(INFO, "event-loop: disconnected from wayland");
			break;
		}
		if (pollfds[EVENT_LOOP_PIPEWIRE].revents & POLLHUP) {
			logprint(INFO, "event-loop: disconnected from pipewire");
			break;
		}

		// sd-bus sets events=0 if it already has messages to process
		if (pollfds[EVENT_LOOP_DBUS].revents ||
				pollfds[EVENT_LOOP_DBUS].events == 0) {
			logprint(TRACE, "event-loop: got dbus event");
			do {
				ret = sd_bus_process(state.bus, NULL);
			} while (ret > 0);
			if (ret < 0) {
				logprint(ERROR, "sd_bus_process failed: %s", strerror(-ret));
				goto error;
			}
		}

		if (pollfds[EVENT_LOOP_WAYLAND].revents & POLLIN) {
			logprint(TRACE, "event-loop: got wayland event");
			ret = wl_display_dispatch(state.wl_display);
			if (ret < 0) {
				logprint(ERROR, "wl_display_dispatch failed: %s", strerror(errno));
				goto error;
			}
		}

		if (pollfds[EVENT_LOOP_PIPEWIRE].revents & POLLIN) {
			logprint(TRACE, "event-loop: got pipewire event");
			ret = pw_loop_iterate(state.pw_loop, 0);
			if (ret < 0) {
				logprint(ERROR, "pw_loop_iterate failed: %s", spa_strerror(ret));
				goto error;
			}
		}

		if (pollfds[EVENT_LOOP_TIMER].revents & POLLIN) {
			logprint(TRACE, "event-loop: got a timer event");

			int timer_fd = pollfds[EVENT_LOOP_TIMER].fd;
			uint64_t expirations;
			ssize_t n = read(timer_fd, &expirations, sizeof(expirations));
			if (n < 0) {
				logprint(ERROR, "failed to read from timer FD\n");
				goto error;
			}

			struct xdpw_timer *timer = state.next_timer;
			if (timer != NULL) {
				xdpw_event_loop_timer_func_t func = timer->func;
				void *user_data = timer->user_data;
				xdpw_destroy_timer(timer);

				func(user_data);
			}
		}

		do {
			ret = wl_display_dispatch_pending(state.wl_display);
			wl_display_flush(state.wl_display);
		} while (ret > 0);

		sd_bus_flush(state.bus);
	}

	// TODO: cleanup
	finish_config(&config);
	free(configfile);

	return EXIT_SUCCESS;

error:
	sd_bus_unref(bus);
	pw_loop_leave(state.pw_loop);
	pw_loop_destroy(state.pw_loop);
	wl_display_disconnect(state.wl_display);
	return EXIT_FAILURE;
}
