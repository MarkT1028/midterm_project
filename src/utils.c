#define _GNU_SOURCE
#include "utils.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/utsname.h>
#include <time.h>

#ifndef MAX_MSG_SIZE
#define MAX_MSG_SIZE (1024*1024) // 1MB
#endif

static log_level_t g_log_level = LOG_INFO;

void log_init(log_level_t level) {
    g_log_level = level;
}

void log_msg(log_level_t level, const char *fmt, ...) {
    if (level > g_log_level) return;
    const char *lvl;
    switch(level) {
        case LOG_ERROR: lvl = "ERROR"; break;
        case LOG_WARN: lvl = "WARN"; break;
        case LOG_INFO: lvl = "INFO"; break;
        case LOG_DEBUG: lvl = "DEBUG"; break;
        default: lvl = "LOG"; break;
    }
    va_list ap;
    va_start(ap, fmt);
    time_t t = time(NULL);
    struct tm tm; localtime_r(&t, &tm);
    char timestr[64];
    strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", &tm);
    fprintf(stderr, "[%s] %s: ", timestr, lvl);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

ssize_t safe_read(int fd, void *buf, size_t count) {
    size_t left = count;
    char *p = buf;
    while (left > 0) {
        ssize_t r = read(fd, p, left);
        if (r < 0) {
            if (errno == EINTR) continue;
            return -1;
        } else if (r == 0) {
            return count - left; // EOF
        }
        left -= r;
        p += r;
    }
    return count;
}

ssize_t safe_write(int fd, const void *buf, size_t count) {
    size_t left = count;
    const char *p = buf;
    while (left > 0) {
        ssize_t w = write(fd, p, left);
        if (w < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        left -= w;
        p += w;
    }
    return count;
}

int send_msg(int fd, const void *buf, uint32_t len) {
    if (len > MAX_MSG_SIZE) return -1;
    uint32_t netlen = htonl(len);
    if (safe_write(fd, &netlen, sizeof(netlen)) != sizeof(netlen)) return -1;
    if (len>0 && safe_write(fd, buf, len) != (ssize_t)len) return -1;
    return 0;
}

int recv_msg(int fd, void **out_buf, uint32_t *out_len) {
    uint32_t netlen;
    ssize_t r = safe_read(fd, &netlen, sizeof(netlen));
    if (r == 0) return 1; // EOF
    if (r != sizeof(netlen)) return -1;
    uint32_t len = ntohl(netlen);
    if (len > MAX_MSG_SIZE) return -1;
    char *buf = malloc(len+1);
    if (!buf) return -1;
    if (len>0) {
        ssize_t rr = safe_read(fd, buf, len);
        if (rr != (ssize_t)len) { free(buf); return -1; }
    }
    buf[len] = '\0';
    *out_buf = buf; *out_len = len;
    return 0;
}

char *read_file_first_n(const char *path, size_t nlines) {
    FILE *f = fopen(path, "r");
    if (!f) return NULL;
    size_t cap = 4096;
    char *out = malloc(cap);
    if (!out) { fclose(f); return NULL; }
    out[0] = '\0';
    size_t used = 0;
    char line[1024];
    size_t cnt = 0;
    while (fgets(line, sizeof(line), f)) {
        if (cnt++ >= nlines) break;
        size_t len = strlen(line);
        if (used + len + 1 > cap) {
            cap *= 2;
            out = realloc(out, cap);
            if (!out) { fclose(f); return NULL; }
        }
        memcpy(out+used, line, len);
        used += len;
        out[used] = '\0';
    }
    fclose(f);
    return out;
}

char *get_sysinfo(void) {
    struct utsname u;
    if (uname(&u) < 0) {
        return strdup("uname failed\n");
    }
    char header[512];
    snprintf(header, sizeof(header), "System: %s\nNode: %s\nRelease: %s\nVersion: %s\nMachine: %s\n",
             u.sysname, u.nodename, u.release, u.version, u.machine);
    char *meminfo = read_file_first_n("/proc/meminfo", 6);
    size_t total_len = strlen(header) + (meminfo?strlen(meminfo):0) + 128;
    char *out = malloc(total_len);
    if (!out) { free(meminfo); return strdup("alloc fail\n"); }
    snprintf(out, total_len, "%s\n/proc/meminfo (first lines):\n%s\n", header, meminfo?meminfo:"(no meminfo)\n");
    free(meminfo);
    return out;
}
