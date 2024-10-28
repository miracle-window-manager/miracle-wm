#include "timespec_util.h"
#include <time.h>

void timespec_add(struct timespec *t, int64_t delta_ns) {
	int delta_ns_low = delta_ns % TIMESPEC_NSEC_PER_SEC;
	int delta_s_high = delta_ns / TIMESPEC_NSEC_PER_SEC;

	t->tv_sec += delta_s_high;

	t->tv_nsec += (long)delta_ns_low;
	if (t->tv_nsec >= TIMESPEC_NSEC_PER_SEC) {
		t->tv_nsec -= TIMESPEC_NSEC_PER_SEC;
		++t->tv_sec;
	}
}

bool timespec_less(struct timespec *t1, struct timespec *t2) {
	if (t1->tv_sec != t2->tv_sec) {
		return t1->tv_sec < t2->tv_sec;
	}
	return t1->tv_nsec < t2->tv_nsec;
}

bool timespec_is_zero(struct timespec *t) {
	return t->tv_sec == 0 && t->tv_nsec == 0;
}

int64_t timespec_diff_ns(struct timespec *t1, struct timespec *t2) {
	int64_t s = t1->tv_sec - t2->tv_sec;
	int64_t ns = t1->tv_nsec - t2->tv_nsec;

	return s * TIMESPEC_NSEC_PER_SEC + ns;
}
