#ifndef ACTIVATION_DB_H
#define ACTIVATION_DB_H

#include "common.h"

/**
 * Initialize activation code database
 */
void init_activation_db(void);

/**
 * Find activation code by code string
 * @return Pointer to ActivationCodeInfo or NULL if not found
 */
ActivationCodeInfo* find_activation_code(const char *code);

/**
 * Validate activation code
 * @return Response code (RESPONSE_OK, RESPONSE_INVALID_CODE, etc.)
 */
int validate_code(const char *code, const char *device_id);

/**
 * Get remaining valid days for code
 * @return Days remaining, -1 if never expires, -2 if not found
 */
int get_remaining_days(const char *code);

#endif /* ACTIVATION_DB_H */
