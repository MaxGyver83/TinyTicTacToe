#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <stdint.h>

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

void debug(const char *fmt, ...);
void info(const char *fmt, ...);
void error(const char *fmt, ...);

int64_t get_timestamp(void);
void sleep_milliseconds(int ms);
int random_int(int min, int max);
bool endswith(const char *suffix, const char *str);

#endif // UTILS_H
