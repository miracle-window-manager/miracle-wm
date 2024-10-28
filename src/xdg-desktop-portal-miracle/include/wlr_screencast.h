#ifndef WLR_SCREENCAST_H
#define WLR_SCREENCAST_H

#include "screencast_common.h"

#define WL_OUTPUT_VERSION 4

#define WL_SHM_VERSION 1

#define LINUX_DMABUF_VERSION 4
#define LINUX_DMABUF_VERSION_MIN 3

#define XDG_OUTPUT_VERSION 3
#define XDG_OUTPUT_VERSION_MIN 1

struct xdpw_state;

int xdpw_wlr_screencopy_init(struct xdpw_state *state);
void xdpw_wlr_screencopy_finish(struct xdpw_screencast_context *ctx);

bool xdpw_wlr_target_chooser(struct xdpw_screencast_context *ctx, struct xdpw_screencast_target *target);
bool xdpw_wlr_target_from_data(struct xdpw_screencast_context *ctx, struct xdpw_screencast_target *target,
		struct xdpw_screencast_restore_data *data);

void xdpw_wlr_frame_capture(struct xdpw_screencast_instance *cast);
int xdpw_wlr_session_init(struct xdpw_screencast_instance *cast);
void xdpw_wlr_session_close(struct xdpw_screencast_instance *cast);

#endif
