#ifndef VERIFY_SAV_H
#define VERIFY_SAV_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize hidden storage for license state.
 * Returns 0 on success.
 */
int verify_sav_init(const char *base_path);

/**
 * Store verification status (1=success,0=failure) to hidden file.
 * Returns 0 on success.
 */
int verify_sav_set(int status);

/**
 * Load cached status from hidden file. Returns 0 on success and stores status in out_status.
 * Returns -1 if not found or parse error.
 */
int verify_sav_get(int *out_status);

/**
 * Remove cached state (for testing / reset)
 */
int verify_sav_clear(void);

#ifdef __cplusplus
}
#endif

#endif /* VERIFY_SAV_H */