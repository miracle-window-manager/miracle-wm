#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "xdpw.h"
#include "screencast.h"
#include "logger.h"

static const char interface_name[] = "org.freedesktop.impl.portal.Session";

static int method_close(sd_bus_message *msg, void *data,
		sd_bus_error *ret_error) {
	int ret = 0;
	struct xdpw_session *sess = data;
	logprint(INFO, "dbus: session closed");

	sd_bus_message *reply = NULL;
	ret = sd_bus_message_new_method_return(msg, &reply);
	if (ret < 0) {
		return ret;
	}

	ret = sd_bus_send(NULL, reply, NULL);
	if (ret < 0) {
		return ret;
	}

	sd_bus_message_unref(reply);

	xdpw_session_destroy(sess);

	return 0;
}

static const sd_bus_vtable session_vtable[] = {
	SD_BUS_VTABLE_START(0),
	SD_BUS_METHOD("Close", "", "", method_close, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_VTABLE_END
};

struct xdpw_session *xdpw_session_create(struct xdpw_state *state, sd_bus *bus, char *object_path) {
	struct xdpw_session *sess = calloc(1, sizeof(struct xdpw_session));

	sess->session_handle = object_path;

	if (sd_bus_add_object_vtable(bus, &sess->slot, object_path, interface_name,
			session_vtable, sess) < 0) {
		free(sess);
		logprint(ERROR, "dbus: sd_bus_add_object_vtable failed: %s",
			strerror(-errno));
		return NULL;
	}

	wl_list_insert(&state->xdpw_sessions, &sess->link);
	return sess;
}

void xdpw_session_destroy(struct xdpw_session *sess) {
	logprint(DEBUG, "dbus: destroying session %p", sess);
	if (!sess) {
		return;
	}
	struct xdpw_screencast_instance *cast = sess->screencast_data.screencast_instance;
	if (cast) {
		assert(cast->refcount > 0);
		--cast->refcount;
		logprint(DEBUG, "xdpw: screencast instance %p now has %d references",
			cast, cast->refcount);
		if (cast->refcount < 1) {
			if (cast->frame_state == XDPW_FRAME_STATE_NONE) {
				logprint(TRACE, "xdpw: screencast instance not streaming, destroy it");
				xdpw_screencast_instance_destroy(cast);
			} else if (cast->teardown) {
				logprint(TRACE, "xdpw: screencast instance marked for teardown, destroy it");
				xdpw_screencast_instance_destroy(cast);
			} else {
				logprint(TRACE, "xdpw: screencast instance still streaming, set quit flag");
				cast->quit = true;
			}
		}
	}

	sd_bus_slot_unref(sess->slot);
	wl_list_remove(&sess->link);
	free(sess->session_handle);
	free(sess);
}
