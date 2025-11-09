#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <sys/types.h>
#include <stdint.h>

// log levels
typedef enum { LOG_ERROR=0, LOG_WARN=1, LOG_INFO=2, LOG_DEBUG=3 } log_level_t;

// init logging (runtime controlled by env var or flag)
void log_init(log_level_t level);

// logging
void log_msg(log_level_t level, const char *fmt, ...);

// safe read/write and message helpers (length-prefixed)
ssize_t safe_read(int fd, void *buf, size_t count);
ssize_t safe_write(int fd, const void *buf, size_t count);
int send_msg(int fd, const void *buf, uint32_t len);
int recv_msg(int fd, void **out_buf, uint32_t *out_len);

// get system info (returns a malloc'd string, caller frees)
char *get_sysinfo(void);

#endif // UTILS_H
