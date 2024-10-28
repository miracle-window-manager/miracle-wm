#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include "xdpw.h"
#include "screenshot.h"

static const char object_path[] = "/org/freedesktop/portal/desktop";
static const char interface_name[] = "org.freedesktop.impl.portal.Screenshot";

static bool exec_screenshooter(const char *path) {
	pid_t pid = fork();
	if (pid < 0) {
		perror("fork");
		return false;
	} else if (pid == 0) {
		char *const argv[] = {
			"grim",
			"--",
			(char *)path,
			NULL,
		};
		execvp("grim", argv);

		perror("execvp");
		exit(127);
	}

	int stat;
	if (waitpid(pid, &stat, 0) < 0) {
		perror("waitpid");
		return false;
	}

	return stat == 0;
}
static bool exec_screenshooter_interactive(const char *path) {
	pid_t pid = fork();
	if (pid < 0) {
		perror("fork");
		return false;
	} else if (pid == 0) {
		char cmd[strlen(path) + 25];
		snprintf(cmd, sizeof(cmd), "grim -g \"$(slurp)\" -- %s", path);
		execl("/bin/sh", "/bin/sh", "-c", cmd, NULL);
		perror("execl");
		exit(127);
	}

	int stat;
	if (waitpid(pid, &stat, 0) < 0) {
		perror("waitpid");
		return false;
	}

	return stat == 0;
}

static int method_screenshot(sd_bus_message *msg, void *data,
		sd_bus_error *ret_error) {
	int ret = 0;

	bool interactive = false;

	char *handle, *app_id, *parent_window;
	ret = sd_bus_message_read(msg, "oss", &handle, &app_id, &parent_window);
	if (ret < 0) {
		return ret;
	}

	ret = sd_bus_message_enter_container(msg, 'a', "{sv}");
	if (ret < 0) {
		return ret;
	}
	char *key;
	int inner_ret = 0;
	while ((ret = sd_bus_message_enter_container(msg, 'e', "sv")) > 0) {
		inner_ret = sd_bus_message_read(msg, "s", &key);
		if (inner_ret < 0) {
			return inner_ret;
		}

		if (strcmp(key, "interactive") == 0) {
			int mode;
			sd_bus_message_read(msg, "v", "b", &mode);
			logprint(DEBUG, "dbus: option interactive: %d", mode);
			interactive = mode;
		} else if (strcmp(key, "modal") == 0) {
			int modal;
			sd_bus_message_read(msg, "v", "b", &modal);
			logprint(DEBUG, "dbus: option modal: %d", modal);
		} else {
			logprint(WARN, "dbus: unknown option %s", key);
			sd_bus_message_skip(msg, "v");
		}

		inner_ret = sd_bus_message_exit_container(msg);
		if (inner_ret < 0) {
			return inner_ret;
		}
	}
	if (ret < 0) {
		return ret;
	}
	ret = sd_bus_message_exit_container(msg);
	if (ret < 0) {
		return ret;
	}

	// TODO: cleanup this
	struct xdpw_request *req =
		xdpw_request_create(sd_bus_message_get_bus(msg), handle);
	if (req == NULL) {
		return -ENOMEM;
	}

	// TODO: choose a better path
	const char path[] = "/tmp/out.png";
	const char uri[] = "file:///tmp/out.png";
	if (interactive && !exec_screenshooter_interactive(path)) {
		ret = -1;
		goto destroy_request;
	}
	if (!interactive && !exec_screenshooter(path)) {
		ret = -1;
		goto destroy_request;
	}

	sd_bus_message *reply = NULL;
	ret = sd_bus_message_new_method_return(msg, &reply);
	if (ret < 0) {
		goto unref_reply;
	}

	ret = sd_bus_message_append(reply, "ua{sv}", PORTAL_RESPONSE_SUCCESS, 1, "uri", "s", uri);
	if (ret < 0) {
		goto unref_reply;
	}

	ret = sd_bus_send(NULL, reply, NULL);
	if (ret < 0) {
		goto unref_reply;
	}

unref_reply:
	sd_bus_message_unref(reply);
destroy_request:
	xdpw_request_destroy(req);
	return ret;
}

static bool spawn_chooser(int chooser_out[2]) {
	pid_t pid = fork();
	if (pid < 0) {
		perror("fork");
		return false;
	} else if (pid == 0) {
		close(chooser_out[0]);

		dup2(chooser_out[1], STDOUT_FILENO);
		close(chooser_out[1]);

		execl("/bin/sh", "/bin/sh", "-c", "grim -s 1 -g \"$(slurp -p)\" -t ppm -", NULL);

		perror("execl");
		_exit(127);
	}

	int stat;
	if (waitpid(pid, &stat, 0) < 0) {
		perror("waitpid");
		return false;
	}

	close(chooser_out[1]);
	return stat == 0;
}

static bool exec_color_picker(struct xdpw_ppm_pixel *pixel) {
	int chooser_out[2];
	if (pipe(chooser_out) == -1) {
		perror("pipe chooser_out");
		return false;
	}

	if (!spawn_chooser(chooser_out)) {
		logprint(ERROR, "Selection failed");
		close(chooser_out[0]);
		return false;
	}

	FILE *f = fdopen(chooser_out[0], "r");
	if (f == NULL) {
		perror("fopen pipe chooser_out");
		close(chooser_out[0]);
		return false;
	}

	char *format = NULL;
	size_t len = 1;
	if (getline(&format, &len, f) < 0) {
		goto error_img;
	}
	if (strcmp(format, "P6\n") != 0) {
		goto error_img;
	}
	if (getline(&format, &len, f) < 0) {
		goto error_img;
	}
	if (strcmp(format, "1 1\n") != 0) {
		goto error_img;
	}

	if (fscanf(f, "%d\n", &pixel->max_color_value) != 1) {
		goto error_img;
	}

	unsigned char pixels[3];
	if (fread(pixels, 3, 1, f) != 1) {
		goto error_img;
	}

	pixel->red = pixels[0];
	pixel->green = pixels[1];
	pixel->blue = pixels[2];

	free(format);
	fclose(f);

	return true;

error_img:
	logprint(WARN, "Invalid image format or size");
	free(format);
	fclose(f);
	return false;
}

static int method_pick_color(sd_bus_message *msg, void *data,
		sd_bus_error *ret_error) {

	int ret = 0;

	char *handle, *app_id, *parent_window;
	ret = sd_bus_message_read(msg, "oss", &handle, &app_id, &parent_window);
	if (ret < 0) {
		return ret;
	}

	struct xdpw_request *req =
		xdpw_request_create(sd_bus_message_get_bus(msg), handle);
	if (req == NULL) {
		return -ENOMEM;
	}

	struct xdpw_ppm_pixel pixel = {0};
	if (!exec_color_picker(&pixel)) {
		ret = -1;
		goto destroy_request;
	}

	double red = pixel.red / (pixel.max_color_value * 1.0);
	double green = pixel.green / (pixel.max_color_value * 1.0);
	double blue = pixel.blue / (pixel.max_color_value * 1.0);

	sd_bus_message *reply = NULL;
	ret = sd_bus_message_new_method_return(msg, &reply);
	if (ret < 0) {
		goto unref_reply;
	}

	ret = sd_bus_message_append(reply, "ua{sv}", PORTAL_RESPONSE_SUCCESS, 1, "color", "(ddd)", red, green, blue);
	if (ret < 0) {
		goto unref_reply;
	}

	ret = sd_bus_send(NULL, reply, NULL);
	if (ret < 0) {
		goto unref_reply;
	}

unref_reply:
	sd_bus_message_unref(reply);
destroy_request:
	xdpw_request_destroy(req);
	return ret;
}

static const sd_bus_vtable screenshot_vtable[] = {
	SD_BUS_VTABLE_START(0),
	SD_BUS_METHOD("Screenshot", "ossa{sv}", "ua{sv}", method_screenshot, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_METHOD("PickColor", "ossa{sv}", "ua{sv}", method_pick_color, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_PROPERTY("version", "u", NULL,
		offsetof(struct xdpw_state, screenshot_version),
		SD_BUS_VTABLE_PROPERTY_CONST),
	SD_BUS_VTABLE_END
};

int xdpw_screenshot_init(struct xdpw_state *state) {
	// TODO: cleanup
	sd_bus_slot *slot = NULL;
	return sd_bus_add_object_vtable(state->bus, &slot, object_path, interface_name,
		screenshot_vtable, state);
}
