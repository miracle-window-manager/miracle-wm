#include "logger.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static struct logger_properties logprops;

void init_logger(FILE *dst, enum LOGLEVEL level) {
	logprops.dst = dst;
	logprops.level = level;
}

enum LOGLEVEL get_loglevel(const char *level) {
	if (strcmp(level, "QUIET") == 0) {
		return QUIET;
	} else if (strcmp(level, "ERROR") == 0) {
		return ERROR;
	} else if (strcmp(level, "WARN") == 0) {
		return WARN;
	} else if (strcmp(level, "INFO") == 0) {
		return INFO;
	} else if (strcmp(level, "DEBUG") == 0) {
		return DEBUG;
	} else if (strcmp(level, "TRACE") == 0) {
		return TRACE;
	}

	fprintf(stderr, "Could not understand log level %s\n", level);
	exit(1);
}

static const char *print_loglevel(enum LOGLEVEL loglevel) {
	switch (loglevel) {
	case QUIET:
		return "QUIET";
	case ERROR:
		return "ERROR";
	case WARN:
		return "WARN";
	case INFO:
		return "INFO";
	case DEBUG:
		return "DEBUG";
	case TRACE:
		return "TRACE";
	}
	fprintf(stderr, "Could not find log level %d\n", loglevel);
	abort();
}

void logprint(enum LOGLEVEL level, char *msg, ...) {
	if (!logprops.dst) {
		fprintf(stderr, "Logger has been called, but was not initialized\n");
		abort();
	}

	if (level > logprops.level || level == QUIET) {
		return;
	}
	va_list args;

	char timestr[200];
	time_t t = time(NULL);
	struct tm *tmp = localtime(&t);

	if (strftime(timestr, sizeof(timestr), "%Y/%m/%d %H:%M:%S", tmp) == 0) {
		fprintf(stderr, "strftime returned 0");
		abort();
	}

	fprintf(logprops.dst, "%s", timestr);
	fprintf(logprops.dst, " ");
	fprintf(logprops.dst, "[%s]", print_loglevel(level));
	fprintf(logprops.dst, " - ");

	va_start(args, msg);
	vfprintf(logprops.dst, msg, args);
	va_end(args);


	fprintf(logprops.dst, "\n");
	fflush(logprops.dst);
}
