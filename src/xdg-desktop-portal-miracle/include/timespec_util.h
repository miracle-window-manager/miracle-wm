#ifndef TIMESPEC_UTIL_H
#define TIMESPEC_UTIL_H

#include <time.h>
#include <stdbool.h>
#include <stdint.h>

#define TIMESPEC_NSEC_PER_SEC 1000000000L

void timespec_add(struct timespec *t, int64_t delta_ns);

bool timespec_less(struct timespec *t1, struct timespec *t2);

bool timespec_is_zero(struct timespec *t);

int64_t timespec_diff_ns(struct timespec *t1, struct timespec *t2);

#endif
