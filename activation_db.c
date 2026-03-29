#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_CODES 1000
#define DEVICE_LOG_FILE "activation_log.txt"

/* In-memory database */
static ActivationCodeInfo g_codes_db[MAX_CODES];
static int g_codes_count = 0;

/**
 * Generate a sample activation code database with test codes
 */
void init_activation_db(void) {
    time_t now = time(NULL);

    g_codes_count = 0;

    /* Test code 1: Never expires, unlimited devices */
    snprintf(g_codes_db[g_codes_count].code, MAX_CODE_LEN, "VALID_CODE");
    g_codes_db[g_codes_count].created_time = now;
    g_codes_db[g_codes_count].expire_time = 0;  /* No expiry */
    g_codes_db[g_codes_count].max_devices = -1; /* Unlimited */
    g_codes_count++;

    /* Test code 2: Expires in 30 days, max 5 devices */
    snprintf(g_codes_db[g_codes_count].code, MAX_CODE_LEN, "CODE_ABC123");
    g_codes_db[g_codes_count].created_time = now;
    g_codes_db[g_codes_count].expire_time = now + (30 * 24 * 3600);
    g_codes_db[g_codes_count].max_devices = 5;
    g_codes_count++;

    /* Test code 3: Expired code */
    snprintf(g_codes_db[g_codes_count].code, MAX_CODE_LEN, "EXPIRED_CODE");
    g_codes_db[g_codes_count].created_time = now - (60 * 24 * 3600);
    g_codes_db[g_codes_count].expire_time = now - (30 * 24 * 3600);
    g_codes_db[g_codes_count].max_devices = 10;
    g_codes_count++;

    printf("[db] Initialized %d activation codes\n", g_codes_count);
}

/**
 * Find activation code in database
 */
ActivationCodeInfo* find_activation_code(const char *code) {
    if (!code) return NULL;

    for (int i = 0; i < g_codes_count; i++) {
        if (strcmp(g_codes_db[i].code, code) == 0) {
            return &g_codes_db[i];
        }
    }

    return NULL;
}

/**
 * Check if code is valid
 */
int validate_code(const char *code, const char *device_id) {
    if (!code || !device_id) {
        fprintf(stderr, "[db] Invalid input\n");
        return RESPONSE_SERVER_ERROR;
    }

    ActivationCodeInfo *ac = find_activation_code(code);
    if (!ac) {
        fprintf(stderr, "[db] Code not found: %s\n", code);
        return RESPONSE_INVALID_CODE;
    }

    /* Check expiration */
    time_t now = time(NULL);
    if (ac->expire_time > 0 && now > ac->expire_time) {
        fprintf(stderr, "[db] Code expired: %s\n", code);
        return RESPONSE_EXPIRED;
    }

    /* Check device limit (simplified - in real scenario would query device database) */
    if (ac->max_devices > 0) {
        int active_devices = rand() % (ac->max_devices + 1); /* Simulated */
        if (active_devices >= ac->max_devices) {
            fprintf(stderr, "[db] Device limit reached: %s\n", code);
            return RESPONSE_DEVICE_LIMIT;
        }
    }

    /* Log activation */
    FILE *log_file = fopen(DEVICE_LOG_FILE, "a");
    if (log_file) {
        fprintf(log_file, "[%ld] Device: %s, Code: %s, Status: OK\n",
                time(NULL), device_id, code);
        fclose(log_file);
    }

    return RESPONSE_OK;
}

/**
 * Get remaining days for a code
 */
int get_remaining_days(const char *code) {
    ActivationCodeInfo *ac = find_activation_code(code);
    if (!ac || ac->expire_time == 0) {
        return -1; /* Never expires */
    }

    time_t now = time(NULL);
    return (ac->expire_time - now) / 86400;
}
