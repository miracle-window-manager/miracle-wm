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
#include "wlr_screencopy.h"
#include "xdpw.h"
#include "logger.h"

void xdpw_wlr_frame_capture(struct xdpw_screencast_instance *cast) {
	if (cast->ctx->screencopy_manager) {
		xdpw_wlr_sc_frame_capture(cast);
	}
}

void xdpw_wlr_session_close(struct xdpw_screencast_instance *cast) {
	if (cast->ctx->screencopy_manager) {
		xdpw_wlr_sc_session_close(cast);
	}
}

int xdpw_wlr_session_init(struct xdpw_screencast_instance *cast) {
	if (cast->ctx->screencopy_manager) {
		return xdpw_wlr_sc_session_init(cast);
	}
	return -1;
}

static void wlr_output_handle_geometry(void *data, struct wl_output *wl_output,
		int32_t x, int32_t y, int32_t phys_width, int32_t phys_height,
		int32_t subpixel, const char *make, const char *model, int32_t transform) {
	struct xdpw_wlr_output *output = data;
	output->make = strdup(make);
	output->model = strdup(model);
	output->transformation = transform;
}

static void wlr_output_handle_mode(void *data, struct wl_output *wl_output,
		uint32_t flags, int32_t width, int32_t height, int32_t refresh) {
	if (flags & WL_OUTPUT_MODE_CURRENT) {
		struct xdpw_wlr_output *output = data;
		output->framerate = (float)refresh/1000;
	}
}

static void wlr_output_handle_done(void *data, struct wl_output *wl_output) {
	/* Nothing to do */
}

static void wlr_output_handle_scale(void *data, struct wl_output *wl_output,
		int32_t factor) {
	/* Nothing to do */
}

static void wlr_output_handle_name(void *data, struct wl_output *wl_output,
		const char *name) {
	struct xdpw_wlr_output *output = data;
	output->name = strdup(name);
}

static void wlr_output_handle_description(void *data, struct wl_output *wl_output,
		const char *description) {
	/* Nothing to do */
}

static const struct wl_output_listener wlr_output_listener = {
	.geometry = wlr_output_handle_geometry,
	.mode = wlr_output_handle_mode,
	.done = wlr_output_handle_done,
	.scale = wlr_output_handle_scale,
	.name = wlr_output_handle_name,
	.description = wlr_output_handle_description,
};

static void xdg_output_handle_logical_position(void *data, struct zxdg_output_v1 *xdg_output_v1,
		int32_t x, int32_t y) {
	struct xdpw_wlr_output *output = data;
	output->x = x;
	output->y = y;
}

static void xdg_output_handle_logical_size(void *data, struct zxdg_output_v1 *xdg_output_v1,
		int32_t width, int32_t height) {
	struct xdpw_wlr_output *output = data;
	output->width = width;
	output->height = height;
}

static void xdg_output_handle_done(void *data, struct zxdg_output_v1 *xdg_output_v1) {
	/* Nothing to do */
}

static void xdg_output_handle_name(void *data, struct zxdg_output_v1 *xdg_output_v1,
		const char *name) {
	/* Nothing to do */
}

static void xdg_output_handle_description(void *data, struct zxdg_output_v1 *xdg_output_v1,
		const char *description) {
	/* Nothing to do */
}

static const struct zxdg_output_v1_listener xdg_output_listener = {
	.logical_position = xdg_output_handle_logical_position,
	.logical_size = xdg_output_handle_logical_size,
	.done = xdg_output_handle_done,
	.name = xdg_output_handle_name,
	.description = xdg_output_handle_description,
};

static struct xdpw_wlr_output *xdpw_wlr_output_first(struct wl_list *output_list) {
	struct xdpw_wlr_output *output, *tmp;
	wl_list_for_each_safe(output, tmp, output_list, link) {
		return output;
	}
	return NULL;
}

static struct xdpw_wlr_output *xdpw_wlr_output_find_by_name(struct wl_list *output_list,
		const char *name) {
	struct xdpw_wlr_output *output, *tmp;
	wl_list_for_each_safe(output, tmp, output_list, link) {
		if (strcmp(output->name, name) == 0) {
			return output;
		}
	}
	return NULL;
}

static struct xdpw_wlr_output *xdpw_wlr_output_find(struct xdpw_screencast_context *ctx,
		struct wl_output *out, uint32_t id) {
	struct xdpw_wlr_output *output, *tmp;
	wl_list_for_each_safe(output, tmp, &ctx->output_list, link) {
		if ((output->output == out) || (output->id == id)) {
			return output;
		}
	}
	return NULL;
}

static pid_t spawn_chooser(char *cmd, int chooser_in[2], int chooser_out[2]) {
	logprint(TRACE,
			"exec chooser called: cmd %s, pipe chooser_in (%d,%d), pipe chooser_out (%d,%d)",
			cmd, chooser_in[0], chooser_in[1], chooser_out[0], chooser_out[1]);
	pid_t pid = fork();

	if (pid < 0) {
		perror("fork");
		return pid;
	} else if (pid == 0) {
		close(chooser_in[1]);
		close(chooser_out[0]);

		dup2(chooser_in[0], STDIN_FILENO);
		dup2(chooser_out[1], STDOUT_FILENO);
		close(chooser_in[0]);
		close(chooser_out[1]);

		execl("/bin/sh", "/bin/sh", "-c", cmd, NULL);

		perror("execl");
		_exit(127);
	}

	close(chooser_in[0]);
	close(chooser_out[1]);

	return pid;
}

static bool wait_chooser(pid_t pid) {
	int status;
	if (waitpid(pid ,&status, 0) != -1 && WIFEXITED(status)) {
		return WEXITSTATUS(status) != 127;
	}
	return false;
}

static bool wlr_output_chooser(struct xdpw_output_chooser *chooser,
		struct wl_list *output_list, struct xdpw_wlr_output **output) {
	logprint(DEBUG, "wlroots: output chooser called");
	struct xdpw_wlr_output *out;
	size_t name_size = 0;
	char *name = NULL;
	*output = NULL;

	int chooser_in[2]; //p -> c
	int chooser_out[2]; //c -> p

	if (pipe(chooser_in) == -1) {
		perror("pipe chooser_in");
		logprint(ERROR, "Failed to open pipe chooser_in");
		goto error_chooser_in;
	}
	if (pipe(chooser_out) == -1) {
		perror("pipe chooser_out");
		logprint(ERROR, "Failed to open pipe chooser_out");
		goto error_chooser_out;
	}

	pid_t pid = spawn_chooser(chooser->cmd, chooser_in, chooser_out);
	if (pid < 0) {
		logprint(ERROR, "Failed to fork chooser");
		goto error_fork;
	}

	switch (chooser->type) {
	case XDPW_CHOOSER_DMENU:;
		FILE *f = fdopen(chooser_in[1], "w");
		if (f == NULL) {
			perror("fdopen pipe chooser_in");
			logprint(ERROR, "Failed to create stream writing to pipe chooser_in");
			goto error_fork;
		}
		wl_list_for_each(out, output_list, link) {
			fprintf(f, "%s\n", out->name);
		}
		fclose(f);
		break;
	default:
		close(chooser_in[1]);
	}

	if (!wait_chooser(pid)) {
		close(chooser_out[0]);
		return false;
	}

	FILE *f = fdopen(chooser_out[0], "r");
	if (f == NULL) {
		perror("fdopen pipe chooser_out");
		logprint(ERROR, "Failed to create stream reading from pipe chooser_out");
		close(chooser_out[0]);
		goto end;
	}

	ssize_t nread = getline(&name, &name_size, f);
	fclose(f);
	if (nread < 0) {
		perror("getline failed");
		goto end;
	}

	//Strip newline
	char *p = strchr(name, '\n');
	if (p != NULL) {
		*p = '\0';
	}

	logprint(TRACE, "wlroots: output chooser %s selects output %s", chooser->cmd, name);
	wl_list_for_each(out, output_list, link) {
		if (strcmp(out->name, name) == 0) {
			*output = out;
			break;
		}
	}
	free(name);

end:
	return true;

error_fork:
	close(chooser_out[0]);
	close(chooser_out[1]);
error_chooser_out:
	close(chooser_in[0]);
	close(chooser_in[1]);
error_chooser_in:
	*output = NULL;
	return false;
}

static struct xdpw_wlr_output *wlr_output_chooser_default(struct wl_list *output_list) {
	logprint(DEBUG, "wlroots: output chooser called");
	struct xdpw_output_chooser default_chooser[] = {
		{XDPW_CHOOSER_SIMPLE, "slurp -f %o -or"},
		{XDPW_CHOOSER_DMENU, "wofi -d -n --prompt='Select the monitor to share:'"},
		{XDPW_CHOOSER_DMENU, "bemenu --prompt='Select the monitor to share:'"},
	};

	size_t N = sizeof(default_chooser)/sizeof(default_chooser[0]);
	struct xdpw_wlr_output *output = NULL;
	bool ret;
	for (size_t i = 0; i<N; i++) {
		ret = wlr_output_chooser(&default_chooser[i], output_list, &output);
		if (!ret) {
			logprint(DEBUG, "wlroots: output chooser %s not found. Trying next one.",
					default_chooser[i].cmd);
			continue;
		}
		if (output != NULL) {
			logprint(DEBUG, "wlroots: output chooser selects %s", output->name);
		} else {
			logprint(DEBUG, "wlroots: output chooser canceled");
		}
		return output;
	}
	return xdpw_wlr_output_first(output_list);
}

static struct xdpw_wlr_output *xdpw_wlr_output_chooser(struct xdpw_screencast_context *ctx) {
	switch (ctx->state->config->screencast_conf.chooser_type) {
	case XDPW_CHOOSER_DEFAULT:
		return wlr_output_chooser_default(&ctx->output_list);
	case XDPW_CHOOSER_NONE:
		if (ctx->state->config->screencast_conf.output_name) {
			return xdpw_wlr_output_find_by_name(&ctx->output_list, ctx->state->config->screencast_conf.output_name);
		} else {
			return xdpw_wlr_output_first(&ctx->output_list);
		}
	case XDPW_CHOOSER_DMENU:
	case XDPW_CHOOSER_SIMPLE:;
		struct xdpw_wlr_output *output = NULL;
		if (!ctx->state->config->screencast_conf.chooser_cmd) {
			logprint(ERROR, "wlroots: no output chooser given");
			goto end;
		}
		struct xdpw_output_chooser chooser = {
			ctx->state->config->screencast_conf.chooser_type,
			ctx->state->config->screencast_conf.chooser_cmd
		};
		logprint(DEBUG, "wlroots: output chooser %s (%d)", chooser.cmd, chooser.type);
		bool ret = wlr_output_chooser(&chooser, &ctx->output_list, &output);
		if (!ret) {
			logprint(ERROR, "wlroots: output chooser %s failed", chooser.cmd);
			goto end;
		}
		if (output) {
			logprint(DEBUG, "wlroots: output chooser selects %s", output->name);
		} else {
			logprint(DEBUG, "wlroots: output chooser canceled");
		}
		return output;
	}
end:
	return NULL;
}

bool xdpw_wlr_target_chooser(struct xdpw_screencast_context *ctx, struct xdpw_screencast_target *target) {
	target->output = xdpw_wlr_output_chooser(ctx);
	return target->output != NULL;
}

bool xdpw_wlr_target_from_data(struct xdpw_screencast_context *ctx, struct xdpw_screencast_target *target,
		struct xdpw_screencast_restore_data *data) {
	struct xdpw_wlr_output *out = NULL;
	out = xdpw_wlr_output_find_by_name(&ctx->output_list, data->output_name);

	if (!out) {
		return false;
	}
	target->output = out;
	return true;
}

static void wlr_remove_output(struct xdpw_wlr_output *out) {
	free(out->name);
	free(out->make);
	free(out->model);
	if (out->xdg_output) {
		zxdg_output_v1_destroy(out->xdg_output);
	}
	wl_output_destroy(out->output);
	wl_list_remove(&out->link);
	free(out);
}

static void wlr_format_modifier_pair_add(struct xdpw_screencast_context *ctx,
		uint32_t format, uint64_t modifier) {
	struct xdpw_format_modifier_pair *fm_pair;
	wl_array_for_each(fm_pair, &ctx->format_modifier_pairs) {
		if (fm_pair->fourcc == format && fm_pair->modifier == modifier) {
			logprint(TRACE, "wlroots: skipping duplicated format %u (%lu)", fm_pair->fourcc, fm_pair->modifier);
			return;
		}
	}

	fm_pair = wl_array_add(&ctx->format_modifier_pairs, sizeof(struct xdpw_format_modifier_pair));
	fm_pair->fourcc = format;
	fm_pair->modifier = modifier;
	logprint(TRACE, "wlroots: format %u (%lu)", fm_pair->fourcc, fm_pair->modifier);
}

static void linux_dmabuf_handle_modifier(void *data,
		struct zwp_linux_dmabuf_v1 *zwp_linux_dmabuf_v1,
		uint32_t format, uint32_t modifier_hi, uint32_t modifier_lo) {
	struct xdpw_screencast_context *ctx = data;

	logprint(TRACE, "wlroots: linux_dmabuf_handle_modifier called");

	uint64_t modifier = (((uint64_t)modifier_hi) << 32) | modifier_lo;
	wlr_format_modifier_pair_add(ctx, format, modifier);
}

static void noop() {
       // This space intentionally left blank
}

static const struct zwp_linux_dmabuf_v1_listener linux_dmabuf_listener = {
	.format = noop,
	.modifier = linux_dmabuf_handle_modifier,
};

static void linux_dmabuf_feedback_handle_main_device(void *data,
		struct zwp_linux_dmabuf_feedback_v1 *zwp_linux_dmabuf_feedback_v1, struct wl_array *device_arr) {
	struct xdpw_screencast_context *ctx = data;

	logprint(DEBUG, "wlroots: linux_dmabuf_feedback_handle_main_device called");

	assert(ctx->gbm == NULL);

	dev_t device;
	assert(device_arr->size == sizeof(device));
	memcpy(&device, device_arr->data, sizeof(device));

	drmDevice *drmDev;
	if (drmGetDeviceFromDevId(device, /* flags */ 0, &drmDev) != 0) {
		logprint(WARN, "wlroots: unable to open main device");
		ctx->state->config->screencast_conf.force_mod_linear = true;
		return;
	}
	ctx->gbm = xdpw_gbm_device_create(drmDev);
}

static void linux_dmabuf_feedback_format_table(void *data,
		struct zwp_linux_dmabuf_feedback_v1 *zwp_linux_dmabuf_feedback_v1, int fd, uint32_t size) {
	struct xdpw_screencast_context *ctx = data;

	logprint(DEBUG, "wlroots: linux_dmabuf_feedback_format_table called");

	wl_array_release(&ctx->format_modifier_pairs);
	wl_array_init(&ctx->format_modifier_pairs);

	ctx->feedback_data.format_table_data = mmap(NULL , size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (ctx->feedback_data.format_table_data == MAP_FAILED) {
		ctx->feedback_data.format_table_data = NULL;
		ctx->feedback_data.format_table_size = 0;
		return;
	}
	ctx->feedback_data.format_table_size = size;
}

static void linux_dmabuf_feedback_handle_done(void *data,
		struct zwp_linux_dmabuf_feedback_v1 *zwp_linux_dmabuf_feedback_v1) {
	struct xdpw_screencast_context *ctx = data;

	logprint(DEBUG, "wlroots: linux_dmabuf_feedback_handle_done called");

	if (ctx->feedback_data.format_table_data) {
		munmap(ctx->feedback_data.format_table_data, ctx->feedback_data.format_table_size);
	}
	ctx->feedback_data.format_table_data = NULL;
	ctx->feedback_data.format_table_size = 0;
}

static void linux_dmabuf_feedback_tranche_target_devices(void *data,
		struct zwp_linux_dmabuf_feedback_v1 *zwp_linux_dmabuf_feedback_v1, struct wl_array *device_arr) {
	struct xdpw_screencast_context *ctx = data;

	logprint(DEBUG, "wlroots: linux_dmabuf_feedback_tranche_target_devices called");

	dev_t device;
	assert(device_arr->size == sizeof(device));
	memcpy(&device, device_arr->data, sizeof(device));

	drmDevice *drmDev;
	if (drmGetDeviceFromDevId(device, /* flags */ 0, &drmDev) != 0) {
		return;
	}

	if (ctx->gbm) {
		drmDevice *drmDevRenderer = NULL;
		drmGetDevice2(gbm_device_get_fd(ctx->gbm), /* flags */ 0, &drmDevRenderer);
		ctx->feedback_data.device_used = drmDevicesEqual(drmDevRenderer, drmDev);
	} else {
		ctx->gbm = xdpw_gbm_device_create(drmDev);
		ctx->feedback_data.device_used = ctx->gbm != NULL;
	}
}

static void linux_dmabuf_feedback_tranche_flags(void *data,
		struct zwp_linux_dmabuf_feedback_v1 *zwp_linux_dmabuf_feedback_v1, uint32_t flags) {
	logprint(DEBUG, "wlroots: linux_dmabuf_feedback_tranche_flags called");
}

static void linux_dmabuf_feedback_tranche_formats(void *data,
		struct zwp_linux_dmabuf_feedback_v1 *zwp_linux_dmabuf_feedback_v1, struct wl_array *indices) {
	struct xdpw_screencast_context *ctx = data;

	logprint(DEBUG, "wlroots: linux_dmabuf_feedback_tranche_formats called");

	if (!ctx->feedback_data.device_used || !ctx->feedback_data.format_table_data) {
		return;
	}
	struct fm_entry {
		uint32_t format;
		uint32_t padding;
		uint64_t modifier;
	};
	// An entry in the table has to be 16 bytes long
	assert(sizeof(struct fm_entry) == 16);

	uint32_t n_modifiers = ctx->feedback_data.format_table_size/sizeof(struct fm_entry);
	struct fm_entry *fm_entry = ctx->feedback_data.format_table_data;
	uint16_t *idx;
	wl_array_for_each(idx, indices) {
		if (*idx >= n_modifiers) {
			continue;
		}
		wlr_format_modifier_pair_add(ctx, (fm_entry + *idx)->format, (fm_entry + *idx)->modifier);
	}
}

static void linux_dmabuf_feedback_tranche_done(void *data,
		struct zwp_linux_dmabuf_feedback_v1 *zwp_linux_dmabuf_feedback_v1) {
	struct xdpw_screencast_context *ctx = data;

	logprint(DEBUG, "wlroots: linux_dmabuf_feedback_tranche_done called");

	ctx->feedback_data.device_used = false;
}

static const struct zwp_linux_dmabuf_feedback_v1_listener linux_dmabuf_listener_feedback = {
	.main_device = linux_dmabuf_feedback_handle_main_device,
	.format_table = linux_dmabuf_feedback_format_table,
	.done = linux_dmabuf_feedback_handle_done,
	.tranche_target_device = linux_dmabuf_feedback_tranche_target_devices,
	.tranche_flags = linux_dmabuf_feedback_tranche_flags,
	.tranche_formats = linux_dmabuf_feedback_tranche_formats,
	.tranche_done = linux_dmabuf_feedback_tranche_done,
};

static void wlr_registry_handle_add(void *data, struct wl_registry *reg,
		uint32_t id, const char *interface, uint32_t ver) {
	struct xdpw_screencast_context *ctx = data;

	logprint(DEBUG, "wlroots: interface to register %s  (Version: %u)",interface, ver);
	if (!strcmp(interface, wl_output_interface.name)) {
		struct xdpw_wlr_output *output = calloc(1, sizeof(*output));

		output->id = id;
		logprint(DEBUG, "wlroots: |-- registered to interface %s (Version %u)", interface, WL_OUTPUT_VERSION);
		output->output = wl_registry_bind(reg, id, &wl_output_interface, WL_OUTPUT_VERSION);

		if (ctx->xdg_output_manager) {
			output->xdg_output = zxdg_output_manager_v1_get_xdg_output(
				ctx->xdg_output_manager, output->output);
			zxdg_output_v1_add_listener(output->xdg_output, &xdg_output_listener, output);
		}

		wl_output_add_listener(output->output, &wlr_output_listener, output);
		wl_list_insert(&ctx->output_list, &output->link);
	}

	if (!strcmp(interface, zwlr_screencopy_manager_v1_interface.name)) {
		uint32_t version = ver;
		if (SC_MANAGER_VERSION < ver) {
			version = SC_MANAGER_VERSION;
		} else if (ver < SC_MANAGER_VERSION_MIN) {
			version = SC_MANAGER_VERSION_MIN;
		}
		logprint(DEBUG, "wlroots: |-- registered to interface %s (Version %u)", interface, version);
		ctx->screencopy_manager = wl_registry_bind(
			reg, id, &zwlr_screencopy_manager_v1_interface, version);
	}

	if (strcmp(interface, wl_shm_interface.name) == 0) {
		logprint(DEBUG, "wlroots: |-- registered to interface %s (Version %u)", interface, WL_SHM_VERSION);
		ctx->shm = wl_registry_bind(reg, id, &wl_shm_interface, WL_SHM_VERSION);
	}

	if (strcmp(interface, zwp_linux_dmabuf_v1_interface.name) == 0) {
		uint32_t version = ver;
		if (LINUX_DMABUF_VERSION < ver) {
			version = LINUX_DMABUF_VERSION;
		} else if (LINUX_DMABUF_VERSION_MIN > ver) {
			logprint(INFO, "wlroots: interface %s (Version %u) is required for DMA-BUF screencast", interface, LINUX_DMABUF_VERSION_MIN);
			return;
		}
		logprint(DEBUG, "wlroots: |-- registered to interface %s (Version %u)", interface, version);
		ctx->linux_dmabuf = wl_registry_bind(reg, id, &zwp_linux_dmabuf_v1_interface, version);

		if (version >= 4) {
			ctx->linux_dmabuf_feedback = zwp_linux_dmabuf_v1_get_default_feedback(ctx->linux_dmabuf);
			zwp_linux_dmabuf_feedback_v1_add_listener(ctx->linux_dmabuf_feedback, &linux_dmabuf_listener_feedback, ctx);
		} else {
			zwp_linux_dmabuf_v1_add_listener(ctx->linux_dmabuf, &linux_dmabuf_listener, ctx);
		}
	}

	if (strcmp(interface, zxdg_output_manager_v1_interface.name) == 0
			&& ver >= XDG_OUTPUT_VERSION_MIN) {
		uint32_t version = ver;
		if (XDG_OUTPUT_VERSION < ver) {
			version = XDG_OUTPUT_VERSION;
		}
		logprint(DEBUG, "wlroots: |-- registered to interface %s (Version %u)", interface, version);
		ctx->xdg_output_manager = wl_registry_bind(
			reg, id, &zxdg_output_manager_v1_interface, version);
	}
}

static void wlr_registry_handle_remove(void *data, struct wl_registry *reg,
		uint32_t id) {
	struct xdpw_screencast_context *ctx = data;
	struct xdpw_wlr_output *output = xdpw_wlr_output_find(ctx, NULL, id);
	if (output) {
		logprint(DEBUG, "wlroots: output removed (%s)", output->name);
		struct xdpw_screencast_instance *cast, *tmp;
		wl_list_for_each_safe(cast, tmp, &ctx->screencast_instances, link) {
			if (cast->target->output == output) {
				// screencopy might be in process for this instance
				xdpw_wlr_session_close(cast);
				// instance might be waiting for wakeup by the frame limiter
				struct xdpw_timer *timer, *ttmp;
				wl_list_for_each_safe(timer, ttmp, &cast->ctx->state->timers, link) {
					if (timer->user_data == cast) {
						xdpw_destroy_timer(timer);
					}
				}
				cast->teardown = true;
				xdpw_screencast_instance_teardown(cast);
			}
		}
		wlr_remove_output(output);
	}
}

static const struct wl_registry_listener wlr_registry_listener = {
	.global = wlr_registry_handle_add,
	.global_remove = wlr_registry_handle_remove,
};

int xdpw_wlr_screencopy_init(struct xdpw_state *state) {
	struct xdpw_screencast_context *ctx = &state->screencast;

	// initialize a list of outputs
	wl_list_init(&ctx->output_list);

	// initialize a list of active screencast instances
	wl_list_init(&ctx->screencast_instances);

	// initialize a list of format modifier pairs
	wl_array_init(&ctx->format_modifier_pairs);

	// retrieve registry
	ctx->registry = wl_display_get_registry(state->wl_display);
	wl_registry_add_listener(ctx->registry, &wlr_registry_listener, ctx);

	wl_display_roundtrip(state->wl_display);

	logprint(DEBUG, "wayland: registry listeners run");

	if (ctx->linux_dmabuf_feedback) {
		wl_display_roundtrip(state->wl_display);

		logprint(DEBUG, "wayland: dmabuf_feedback listeners run");
	}

	// make sure our wlroots supports shm protocol
	if (!ctx->shm) {
		logprint(ERROR, "Compositor doesn't support %s!", "wl_shm");
		return -1;
	}

	// make sure our wlroots supports screencopy protocol
	if (!ctx->screencopy_manager) {
		logprint(ERROR, "Compositor doesn't support %s!",
			zwlr_screencopy_manager_v1_interface.name);
		return -1;
	}

	// make sure we have a gbm device
	if (ctx->linux_dmabuf && !ctx->gbm) {
		ctx->gbm = xdpw_gbm_device_create(NULL);
		if (!ctx->gbm) {
			logprint(ERROR, "System doesn't support gbm!");
		}
	}

	if (ctx->xdg_output_manager) {
		struct xdpw_wlr_output *output;
		wl_list_for_each(output, &ctx->output_list, link) {
			if (!output->xdg_output) {
				output->xdg_output = zxdg_output_manager_v1_get_xdg_output(
					ctx->xdg_output_manager, output->output);
				zxdg_output_v1_add_listener(output->xdg_output, &xdg_output_listener, output);
			}
		}
		wl_display_roundtrip(state->wl_display);
		logprint(DEBUG, "wayland: xdg_output listeners run");
	}

	return 0;
}

void xdpw_wlr_screencopy_finish(struct xdpw_screencast_context *ctx) {
	wl_array_release(&ctx->format_modifier_pairs);

	struct xdpw_wlr_output *output, *tmp_o;
	wl_list_for_each_safe(output, tmp_o, &ctx->output_list, link) {
		wl_list_remove(&output->link);
		if (output->xdg_output) {
			zxdg_output_v1_destroy(output->xdg_output);
		}
		wl_output_destroy(output->output);
	}

	struct xdpw_screencast_instance *cast, *tmp_c;
	wl_list_for_each_safe(cast, tmp_c, &ctx->screencast_instances, link) {
		cast->quit = true;
	}

	if (ctx->screencopy_manager) {
		zwlr_screencopy_manager_v1_destroy(ctx->screencopy_manager);
	}
	if (ctx->shm) {
		wl_shm_destroy(ctx->shm);
	}
	if (ctx->gbm) {
		int fd = gbm_device_get_fd(ctx->gbm);
		gbm_device_destroy(ctx->gbm);
		close(fd);
	}
	if (ctx->linux_dmabuf_feedback) {
		zwp_linux_dmabuf_feedback_v1_destroy(ctx->linux_dmabuf_feedback);
	}
	if (ctx->linux_dmabuf) {
		zwp_linux_dmabuf_v1_destroy(ctx->linux_dmabuf);
	}
	if (ctx->xdg_output_manager) {
		zxdg_output_manager_v1_destroy(ctx->xdg_output_manager);
	}
	if (ctx->registry) {
		wl_registry_destroy(ctx->registry);
	}
}
