#ifndef WLR_SCREENCOPY_H
#define WLR_SCREENCOPY_H

#include "screencast_common.h"

#define SC_MANAGER_VERSION 3
#define SC_MANAGER_VERSION_MIN 2

void xdpw_wlr_sc_frame_capture(struct xdpw_screencast_instance *cast);
int xdpw_wlr_sc_session_init(struct xdpw_screencast_instance *cast);
void xdpw_wlr_sc_session_close(struct xdpw_screencast_instance *cast);

#endif
