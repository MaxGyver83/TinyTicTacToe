#define _POSIX_C_SOURCE 200809L
#include "utils.h"
#ifndef X11
#include <android/log.h>      // for __android_log_vprint, android_LogPriority
#endif
#include <time.h>             // for timespec, clock_gettime, nanosleep
#define itimerspec linux_itimerspec
#include <linux/time.h>       // for CLOCK_MONOTONIC
#undef itimerspec
#include <stdarg.h>           // for va_end, va_start, va_list
#include <stdio.h>            // for fprintf, stderr, NULL
#include <stdlib.h>           // for rand, RAND_MAX

#define TAG "com.goodtemperapps.tinytictactoe"

void
debug(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
#ifdef X11
	vprintf(fmt, args);
	printf("\n");
#else
	__android_log_vprint(ANDROID_LOG_DEBUG, TAG, fmt, args);
#endif
	va_end(args);
}

void
info(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
#ifdef X11
	vprintf(fmt, args);
	printf("\n");
#else
	__android_log_vprint(ANDROID_LOG_INFO, TAG, fmt, args);
#endif
	va_end(args);
}

void
error(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
#ifdef X11
	// print error in red
	fprintf(stderr, "\033[0;31mError: ");
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\033[0;0m\n");
#else
	__android_log_vprint(ANDROID_LOG_ERROR, TAG, fmt, args);
#endif
	va_end(args);
}

int64_t
get_timestamp(void)
{
	struct timespec tms;

	if (clock_gettime(CLOCK_MONOTONIC, &tms))
		return 0;
	/* seconds, multiplied with 1 million */
	int64_t microseconds = tms.tv_sec * 1000000;
	/* Add full microseconds */
	microseconds += tms.tv_nsec / 1000;
	/* round up if necessary */
	if (tms.tv_nsec % 1000 >= 500)
		++microseconds;
	return microseconds;
}

void
sleep_milliseconds(int ms)
{
	nanosleep((const struct timespec[]) {{0, ms*1000000L}}, NULL);
}

int
random_int(int min, int max)
{
	return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

