#include "fps_limit.h"
#include "logger.h"
#include "timespec_util.h"
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>

#define FPS_MEASURE_PERIOD_SEC 5.0

void measure_fps(struct fps_limit_state *state, struct timespec *now);

void fps_limit_measure_start(struct fps_limit_state *state, double max_fps) {
	if (max_fps <= 0.0) {
		return;
	}

	clock_gettime(CLOCK_MONOTONIC, &state->frame_last_time);
}

uint64_t fps_limit_measure_end(struct fps_limit_state *state, double max_fps) {
	if (max_fps <= 0.0) {
		return 0;
	}

	// `fps_limit_measure_start` was not called?
	assert(!timespec_is_zero(&state->frame_last_time));

	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	int64_t elapsed_ns = timespec_diff_ns(&now, &state->frame_last_time);

	measure_fps(state, &now);

	int64_t target_ns = (1.0 / max_fps) * TIMESPEC_NSEC_PER_SEC;
	int64_t delay_ns = target_ns - elapsed_ns;
	if (delay_ns > 0) {
		logprint(TRACE, "fps_limit: elapsed time since the last measurement: %u, "
			"target %u, should delay for %u (ns)", elapsed_ns, target_ns, delay_ns);
		return delay_ns;
	} else {
		logprint(TRACE, "fps_limit: elapsed time since the last measurement: %u, "
			"target %u, target not met (ns)", elapsed_ns, target_ns);
		return 0;
	}
}

void measure_fps(struct fps_limit_state *state, struct timespec *now) {
	if (timespec_is_zero(&state->fps_last_time)) {
		state->fps_last_time = *now;
		return;
	}

	state->fps_frame_count++;

	int64_t elapsed_ns = timespec_diff_ns(now, &state->fps_last_time);

	double elapsed_sec = (double) elapsed_ns / (double) TIMESPEC_NSEC_PER_SEC;
	if (elapsed_sec < FPS_MEASURE_PERIOD_SEC) {
		return;
	}

	double avg_frames_per_sec = state->fps_frame_count / elapsed_sec;

	logprint(DEBUG, "fps_limit: average FPS in the last %0.2f seconds: %0.2f",
		elapsed_sec, avg_frames_per_sec);

	state->fps_last_time = *now;
	state->fps_frame_count = 0;
}
