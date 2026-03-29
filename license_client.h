#ifndef LICENSE_CLIENT_H
#define LICENSE_CLIENT_H

/**
 * license_verify - Verify activation code with license server
 * @host: License server hostname/IP
 * @port: License server port
 * @code: Activation code to verify
 * @device_id: Device identifier (MAC address, UUID, etc.)
 * @result: Buffer for server response message
 * @result_size: Size of result buffer
 * 
 * @return: Status code (200 = OK, 400 = Invalid, 401 = Expired, 402 = Device limit, 500 = Error)
 */
int license_verify(const char *host, int port,
                   const char *code, const char *device_id,
                   char *result, size_t result_size);

/**
 * license_verify_simple - Verify using default server (127.0.0.1:22345)
 */
int license_verify_simple(const char *code, const char *device_id,
                         char *result, size_t result_size);

/**
 * license_check - Simple check function that returns 1/0
 */
int license_check(const char *code, const char *device_id);

#endif /* LICENSE_CLIENT_H */
