#ifndef COMMON_H
#define COMMON_H

#include <time.h>
#include <stdint.h>
#include <sodium.h>
#include <string.h>

/* Activation Code Protocol Definitions */
#define PROTOCOL_VERSION 1
#define MAX_CODE_LEN 64
#define MAX_DEVICE_ID_LEN 128
#define MAX_RESPONSE_LEN 512
#define ACTIVATION_DB_FILE "activation_codes.db"
#define SHARED_KEY_SIZE 32

/* Request/Response message types */
#define REQUEST_VERIFY 1
#define RESPONSE_OK 0
#define RESPONSE_INVALID_CODE 1
#define RESPONSE_EXPIRED 2
#define RESPONSE_DEVICE_LIMIT 3
#define RESPONSE_SERVER_ERROR 4

/* Activation Code Info */
typedef struct {
    char code[MAX_CODE_LEN];
    uint64_t created_time;
    uint64_t expire_time;
    int max_devices;
    unsigned char signature[64];
} ActivationCodeInfo;

/* Request structure */
typedef struct {
    uint8_t version;
    uint8_t msg_type;
    char code[MAX_CODE_LEN];
    char device_id[MAX_DEVICE_ID_LEN];
    uint64_t timestamp;
    unsigned char nonce[crypto_secretbox_NONCEBYTES];
} ActivationRequest;

/* Response structure */
typedef struct {
    uint8_t status_code;
    uint64_t timestamp;
    char message[MAX_RESPONSE_LEN];
    unsigned char signature[64];
} ActivationResponse;

/* Global crypto keys */
extern unsigned char g_shared_key[SHARED_KEY_SIZE];
extern unsigned char g_public_key[crypto_sign_PUBLICKEYBYTES];
extern unsigned char g_secret_key[crypto_sign_SECRETKEYBYTES];

/* Encryption/Decryption functions */
int encrypt_msg(unsigned char *msg, size_t msg_len,
                unsigned char *out, unsigned char *nonce);

int decrypt_msg(unsigned char *cipher, size_t cipher_len,
                unsigned char *nonce, unsigned char *out);

/* Signature functions */
int sign_message(unsigned char *msg, size_t msg_len,
                 unsigned char *signature);

int verify_signature(unsigned char *msg, size_t msg_len,
                     unsigned char *signature);

#endif /* COMMON_H */