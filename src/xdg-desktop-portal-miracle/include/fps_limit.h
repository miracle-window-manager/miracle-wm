#ifndef FPS_LIMIT_H
#define FPS_LIMIT_H

#include <stdint.h>
#include <time.h>

struct fps_limit_state {
	struct timespec frame_last_time;
	
	struct timespec fps_last_time;
	uint64_t fps_frame_count;
};

void fps_limit_measure_start(struct fps_limit_state *state, double max_fps);

uint64_t fps_limit_measure_end(struct fps_limit_state *state, double max_fps);

#endif
