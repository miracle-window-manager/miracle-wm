#include "wlr_screencast.h"

#include "linux-dmabuf-unstable-v1-client-protocol.h"
#include "wlr-screencopy-unstable-v1-client-protocol.h"
#include "xdg-output-unstable-v1-client-protocol.h"
#include <drm_fourcc.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <assert.h>
#include <wayland-client-protocol.h>
#include <xf86drm.h>

#include "screencast.h"
#include "pipewire_screencast.h"
#include "xdpw.h"
#include "logger.h"
#include "fps_limit.h"

static void wlr_frame_free(struct xdpw_screencast_instance *cast) {
	if (!cast->wlr_session.wlr_frame) {
		return;
	}
	zwlr_screencopy_frame_v1_destroy(cast->wlr_session.wlr_frame);
	cast->wlr_session.wlr_frame = NULL;
	logprint(TRACE, "wlroots: frame destroyed");
}

static void wlr_frame_finish(struct xdpw_screencast_instance *cast) {
	logprint(TRACE, "wlroots: finish screencopy");

	wlr_frame_free(cast);

	if (cast->quit || cast->err) {
		// TODO: revisit the exit condition (remove quit?)
		// and clean up sessions that still exist if err
		// is the cause of the instance_destroy call
		xdpw_screencast_instance_destroy(cast);
		return;
	}

	if (!cast->pwr_stream_state) {
		cast->frame_state = XDPW_FRAME_STATE_NONE;
		return;
	}

	if (cast->frame_state == XDPW_FRAME_STATE_RENEG) {
		pwr_update_stream_param(cast);
	}

	if (cast->frame_state == XDPW_FRAME_STATE_FAILED) {
		xdpw_pwr_enqueue_buffer(cast);
	}

	if (cast->frame_state == XDPW_FRAME_STATE_SUCCESS) {
		xdpw_pwr_enqueue_buffer(cast);
	}
}

static void wlr_frame_buffer_done(void *data,
		struct zwlr_screencopy_frame_v1 *frame);

static void wlr_frame_buffer(void *data, struct zwlr_screencopy_frame_v1 *frame,
		uint32_t format, uint32_t width, uint32_t height, uint32_t stride) {
	struct xdpw_screencast_instance *cast = data;
	if (!frame) {
		return;
	}

	logprint(TRACE, "wlroots: buffer event handler");
	cast->wlr_session.wlr_frame = frame;

	cast->screencopy_frame_info[WL_SHM].width = width;
	cast->screencopy_frame_info[WL_SHM].height = height;
	cast->screencopy_frame_info[WL_SHM].stride = stride;
	cast->screencopy_frame_info[WL_SHM].size = stride * height;
	cast->screencopy_frame_info[WL_SHM].format = xdpw_format_drm_fourcc_from_wl_shm(format);

	if (zwlr_screencopy_manager_v1_get_version(cast->ctx->screencopy_manager) < 3) {
		wlr_frame_buffer_done(cast, frame);
	}
}

static void wlr_frame_linux_dmabuf(void *data,
		struct zwlr_screencopy_frame_v1 *frame,
		uint32_t format, uint32_t width, uint32_t height) {
	struct xdpw_screencast_instance *cast = data;
	if (!frame) {
		return;
	}

	logprint(TRACE, "wlroots: linux_dmabuf event handler");

	cast->screencopy_frame_info[DMABUF].width = width;
	cast->screencopy_frame_info[DMABUF].height = height;
	cast->screencopy_frame_info[DMABUF].format = format;
}

static void wlr_frame_buffer_done(void *data,
		struct zwlr_screencopy_frame_v1 *frame) {
	struct xdpw_screencast_instance *cast = data;
	if (!frame) {
		return;
	}

	logprint(TRACE, "wlroots: buffer_done event handler");

	if (!cast->initialized) {
		wlr_frame_finish(cast);
		return;
	}

	// Check if announced screencopy information is compatible with pipewire meta
	if ((cast->pwr_format.format != xdpw_format_pw_from_drm_fourcc(cast->screencopy_frame_info[cast->buffer_type].format) &&
			cast->pwr_format.format != xdpw_format_pw_strip_alpha(xdpw_format_pw_from_drm_fourcc(cast->screencopy_frame_info[cast->buffer_type].format))) ||
			cast->pwr_format.size.width != cast->screencopy_frame_info[cast->buffer_type].width ||
			cast->pwr_format.size.height != cast->screencopy_frame_info[cast->buffer_type].height) {
		logprint(DEBUG, "wlroots: pipewire and wlroots metadata are incompatible. Renegotiate stream");
		cast->frame_state = XDPW_FRAME_STATE_RENEG;
		wlr_frame_finish(cast);
		return;
	}

	if (!cast->current_frame.xdpw_buffer) {
		logprint(WARN, "wlroots: no current buffer");
		wlr_frame_finish(cast);
		return;
	}

	assert(cast->current_frame.xdpw_buffer);

	// Check if dequeued buffer is compatible with announced buffer
	if (( cast->buffer_type == WL_SHM &&
				(cast->current_frame.xdpw_buffer->size[0] != cast->screencopy_frame_info[cast->buffer_type].size ||
				cast->current_frame.xdpw_buffer->stride[0] != cast->screencopy_frame_info[cast->buffer_type].stride)) ||
			cast->current_frame.xdpw_buffer->width != cast->screencopy_frame_info[cast->buffer_type].width ||
			cast->current_frame.xdpw_buffer->height != cast->screencopy_frame_info[cast->buffer_type].height) {
		logprint(DEBUG, "wlroots: pipewire buffer has wrong dimensions");
		cast->frame_state = XDPW_FRAME_STATE_FAILED;
		wlr_frame_finish(cast);
		return;
	}

	cast->current_frame.transformation = cast->target->output->transformation;
	logprint(TRACE, "wlroots: transformation %u", cast->current_frame.transformation);

	cast->current_frame.damage_count = 0;
	zwlr_screencopy_frame_v1_copy_with_damage(frame, cast->current_frame.xdpw_buffer->buffer);
	logprint(TRACE, "wlroots: frame copied");

	fps_limit_measure_start(&cast->fps_limit, cast->framerate);
}

static void wlr_frame_flags(void *data, struct zwlr_screencopy_frame_v1 *frame,
		uint32_t flags) {
	struct xdpw_screencast_instance *cast = data;
	if (!frame) {
		return;
	}

	logprint(TRACE, "wlroots: flags event handler");
	cast->current_frame.y_invert = flags & ZWLR_SCREENCOPY_FRAME_V1_FLAGS_Y_INVERT;
}

static void wlr_frame_damage(void *data, struct zwlr_screencopy_frame_v1 *frame,
		uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
	struct xdpw_screencast_instance *cast = data;
	if (!frame) {
		return;
	}

	logprint(TRACE, "wlroots: damage event handler");

	logprint(TRACE, "wlroots: damage %"PRIu32": %"PRIu32",%"PRIu32"x%"PRIu32",%"PRIu32, cast->current_frame.damage_count, x, y, width, height);
	struct xdpw_frame_damage damage = {x, y, width, height};
	if (cast->current_frame.damage_count < 4) {
		cast->current_frame.damage[cast->current_frame.damage_count++] = damage;
	} else {
		cast->current_frame.damage[3] = merge_damage(&cast->current_frame.damage[3], &damage);
	}
}

static void wlr_frame_ready(void *data, struct zwlr_screencopy_frame_v1 *frame,
		uint32_t tv_sec_hi, uint32_t tv_sec_lo, uint32_t tv_nsec) {
	struct xdpw_screencast_instance *cast = data;
	if (!frame) {
		return;
	}

	logprint(TRACE, "wlroots: ready event handler");

	cast->current_frame.tv_sec = ((((uint64_t)tv_sec_hi) << 32) | tv_sec_lo);
	cast->current_frame.tv_nsec = tv_nsec;
	logprint(TRACE, "wlroots: timestamp %"PRIu64":%"PRIu32, cast->current_frame.tv_sec, cast->current_frame.tv_nsec);

	cast->frame_state = XDPW_FRAME_STATE_SUCCESS;

	wlr_frame_finish(cast);
}

static void wlr_frame_failed(void *data,
		struct zwlr_screencopy_frame_v1 *frame) {
	struct xdpw_screencast_instance *cast = data;
	if (!frame) {
		return;
	}

	logprint(TRACE, "wlroots: failed event handler");

	cast->frame_state = XDPW_FRAME_STATE_FAILED;

	wlr_frame_finish(cast);
}

static const struct zwlr_screencopy_frame_v1_listener wlr_frame_listener = {
	.buffer = wlr_frame_buffer,
	.buffer_done = wlr_frame_buffer_done,
	.linux_dmabuf = wlr_frame_linux_dmabuf,
	.flags = wlr_frame_flags,
	.ready = wlr_frame_ready,
	.failed = wlr_frame_failed,
	.damage = wlr_frame_damage,
};

static void wlr_register_cb(struct xdpw_screencast_instance *cast) {
	cast->wlr_session.frame_callback = zwlr_screencopy_manager_v1_capture_output(
		cast->ctx->screencopy_manager, cast->target->with_cursor, cast->target->output->output);

	zwlr_screencopy_frame_v1_add_listener(cast->wlr_session.frame_callback,
		&wlr_frame_listener, cast);
	logprint(TRACE, "wlroots: callbacks registered");
}

void xdpw_wlr_sc_frame_capture(struct xdpw_screencast_instance *cast) {
	logprint(TRACE, "wlroots: start screencopy");
	if (cast->quit || cast->err) {
		xdpw_screencast_instance_destroy(cast);
		return;
	}

	if (cast->initialized && !cast->pwr_stream_state) {
		cast->frame_state = XDPW_FRAME_STATE_NONE;
		return;
	}

	cast->frame_state = XDPW_FRAME_STATE_STARTED;
	wlr_register_cb(cast);
}

void xdpw_wlr_sc_session_close(struct xdpw_screencast_instance *cast) {
	wlr_frame_free(cast);
}

int xdpw_wlr_sc_session_init(struct xdpw_screencast_instance *cast) {
	wlr_register_cb(cast);

	// process at least one frame so that we know
	// some of the metadata required for the pipewire
	// remote state connected event
	wl_display_dispatch(cast->ctx->state->wl_display);
	wl_display_roundtrip(cast->ctx->state->wl_display);

	if (cast->screencopy_frame_info[WL_SHM].format == DRM_FORMAT_INVALID ||
			(cast->ctx->state->screencast_version >= 3 &&
			 cast->screencopy_frame_info[DMABUF].format == DRM_FORMAT_INVALID)) {
		logprint(INFO, "wlroots: unable to receive a valid format from wlr_screencopy");
		return -1;
	}

	return 0;
}
