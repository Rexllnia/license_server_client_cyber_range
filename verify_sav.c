#include "verify_sav.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <string.h>
#include <time.h>
#include <errno.h>

static char g_cache_path[512] = "";

static void int_to_string(long v, char *buf, size_t n) {
    if (n == 0) return;
    char tmp[32];
    int idx = 0;
    if (v == 0) {
        if (n > 1) buf[0] = '0', buf[1] = '\0';
        else buf[0] = '\0';
        return;
    }
    while (v > 0 && idx < (int)sizeof(tmp)-1) {
        tmp[idx++] = '0' + (v % 10);
        v /= 10;
    }
    if (idx >= (int)n) {
        buf[0] = '\0';
        return;
    }
    for (int i = 0; i < idx; i++) buf[i] = tmp[idx - 1 - i];
    buf[idx] = '\0';
}

static long string_to_long(const char *s) {
    long v = 0;
    while (*s >= '0' && *s <= '9') {
        v = v * 10 + (*s - '0');
        s++;
    }
    return v;
}

static void compute_cache_path(const char * base_path, char * out_path, size_t out_size) {
    const char *home = getenv("HOME");
    if (!home) home = "/tmp";

    if (base_path && base_path[0] != '\0') {
        int written = 0;
        if ((size_t)snprintf(out_path, out_size, "%s/%s", home, base_path) < out_size) {
            written = strlen(out_path);
        }
        if (out_size > (size_t)written + 30) {
            strcat(out_path, "/.license_cache");
        }
    } else {
        snprintf(out_path, out_size, "%s/.local/share/.license_cache", home);
    }

    /* Ensure dir exists */
    syscall(SYS_mkdir, out_path, 0700);

    if (out_size > strlen(out_path) + 12) {
        strcat(out_path, "/state.dat");
    }
}

int verify_sav_init(const char *base_path) {
    compute_cache_path(base_path, g_cache_path, sizeof(g_cache_path));
    return 0;
}

int verify_sav_set(int status) {
    if (g_cache_path[0] == '\0') {
        verify_sav_init(NULL);
    }

    int fd = syscall(SYS_open, g_cache_path,
                     O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW,
                     0600);
    if (fd < 0) {
        return -1;
    }

    long now = time(NULL);
    char buf[128];
    int i = 0;
    i = snprintf(buf, sizeof(buf), "%d\t%ld\n", status, now);
    if (i <= 0) {
        syscall(SYS_close, fd);
        return -1;
    }

    long written = 0;
    while (written < i) {
        long w = syscall(SYS_write, fd, buf + written, i - written);
        if (w <= 0) {
            syscall(SYS_close, fd);
            return -1;
        }
        written += w;
    }

    syscall(SYS_close, fd);
    return 0;
}

int verify_sav_get(int *out_status) {
    if (!out_status) return -1;
    if (g_cache_path[0] == '\0') {
        verify_sav_init(NULL);
    }

    int fd = syscall(SYS_open, g_cache_path, O_RDONLY | O_NOFOLLOW);
    if (fd < 0) {
        return -1;
    }

    char buf[128];
    long r = syscall(SYS_read, fd, buf, sizeof(buf) - 1);
    syscall(SYS_close, fd);
    if (r <= 0) return -1;

    buf[r] = '\0';

    int status = -1;
    long saved_time = 0;
    char *p = strchr(buf, '\t');
    if (!p) return -1;
    *p = '\0';
    status = atoi(buf);
    saved_time = string_to_long(p + 1);

    if (time(NULL) - saved_time > 30 * 24 * 3600) {
        return -1;
    }

    *out_status = status;
    return 0;
}

int verify_sav_clear(void) {
    if (g_cache_path[0] == '\0') {
        verify_sav_init(NULL);
    }
    if (syscall(SYS_unlink, g_cache_path) != 0) {
        return -1;
    }
    return 0;
}
