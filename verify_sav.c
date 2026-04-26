#include "verify_sav.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/syscall.h>
#include <time.h>
#include <errno.h>
#define SYSCALL_OBSCURE_STAGE1 \
        "sub $16, %%rsp\n"\
        "xor %%rax, %%rax\n"\
        "mov $3, %%al\n"\
        "shl $2, %%al\n"\
        "add $3, %%al\n"\
        "mov %%al, (%%rsp)\n"\
        "xor %%rax, %%rax\n"\
        "mov $2, %%al\n"\
        "add $3, %%al\n"\
        "mov %%al, 1(%%rsp)\n"\
        "movb $0xc3, 2(%%rsp)\n"
#define SYSCALL_OBSCURE_STAGE2 \
        "call *%%rsp\n"\
        "mov %%rax, %0\n"\
        "add $16, %%rsp\n"
     
#define OBSCURE_SYSCALL0(num) \
    syscall_impl_0(num)
#define OBSCURE_SYSCALL1(num, a1) \
    syscall_impl_1(num, a1)
#define OBSCURE_SYSCALL2(num, a1, a2) \
    syscall_impl_2(num, a1, a2)
#define OBSCURE_SYSCALL3(num, a1, a2, a3) \
    syscall_impl_3(num, a1, a2, a3)
#define OBSCURE_SYSCALL4(num, a1, a2, a3, a4) \
    syscall_impl_4(num, a1, a2, a3, a4)

// 内联函数实现
static inline long syscall_impl_0(long num) {
    long ret;
    asm volatile(
        "movq %1, %%rax\n"
        "syscall\n"
        "movq %%rax, %0"
        : "=r"(ret)
        : "r"(num)
        : "rax", "rcx", "r11", "memory"
    );
    return ret;
}

static inline long syscall_impl_1(long num, long a1) {
    long ret;
    asm volatile(
        SYSCALL_OBSCURE_STAGE1
        "movq %1, %%rax\n"
        "movq %2, %%rdi\n"
        SYSCALL_OBSCURE_STAGE2
        : "=r"(ret)
        : "r"(num), "r"(a1)
        : "rax", "rdi", "rcx", "r11", "memory"
    );
    return ret;
}

static inline long syscall_impl_2(long num, long a1, long a2) {
    long ret;
    asm volatile(
        SYSCALL_OBSCURE_STAGE1
        "movq %1, %%rax\n"
        "movq %2, %%rdi\n"
        "movq %3, %%rsi\n"
        SYSCALL_OBSCURE_STAGE2
        : "=r"(ret)
        : "r"(num), "r"(a1), "r"(a2)
        : "rax", "rdi", "rsi", "rcx", "r11", "memory"
    );
    return ret;
}

static inline long syscall_impl_3(long num, long a1, long a2, long a3) {
    long ret;
    asm volatile(
        SYSCALL_OBSCURE_STAGE1
        "movq %1, %%rax\n"
        "movq %2, %%rdi\n"
        "movq %3, %%rsi\n"
        "movq %4, %%rdx\n"
        SYSCALL_OBSCURE_STAGE2
        : "=r"(ret)
        : "r"(num), "r"(a1), "r"(a2), "r"(a3)
        : "rax", "rdi", "rsi", "rdx", "rcx", "r11", "memory"
    );
    return ret;
}

static inline long syscall_impl_4(long num, long a1, long a2, long a3, long a4) {
    long ret;
    asm volatile(
        SYSCALL_OBSCURE_STAGE1
        "movq %1, %%rax\n"
        "movq %2, %%rdi\n"
        "movq %3, %%rsi\n"
        "movq %4, %%rdx\n"
        "movq %5, %%r10\n"
        SYSCALL_OBSCURE_STAGE2
        : "=r"(ret)
        : "r"(num), "r"(a1), "r"(a2), "r"(a3), "r"(a4)
        : "rax", "rdi", "rsi", "rdx","r10", "rcx", "r11", "memory"
    );
    return ret;
}

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
    OBSCURE_SYSCALL2(SYS_mkdir, (long)out_path, 0700);

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

    int fd = OBSCURE_SYSCALL3(SYS_open, g_cache_path,
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
        OBSCURE_SYSCALL1(SYS_close, fd);
        return -1;
    }

    long written = 0;
    while (written < i) {
        long w = OBSCURE_SYSCALL3(SYS_write, fd, buf + written, i - written);
        if (w <= 0) {
            OBSCURE_SYSCALL1(SYS_close, fd);
            return -1;
        }
        written += w;
    }

    OBSCURE_SYSCALL1(SYS_close, fd);
    return 0;
}

int verify_sav_get(int *out_status) {
    if (!out_status) return -1;
    if (g_cache_path[0] == '\0') {
        verify_sav_init(NULL);
    }

    int fd = OBSCURE_SYSCALL2(SYS_open, g_cache_path, O_RDONLY | O_NOFOLLOW);
    if (fd < 0) {
        return -1;
    }

    char buf[128];
    long r = OBSCURE_SYSCALL3(SYS_read, fd, buf, sizeof(buf) - 1);
    OBSCURE_SYSCALL1(SYS_close, fd);
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
    if (OBSCURE_SYSCALL1(SYS_unlink, g_cache_path) != 0) {
        return -1;
    }
    return 0;
}
