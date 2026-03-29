#include "common.h"
#include <stdio.h>
#include <stdlib.h>

/* Global crypto keys - should be generated or loaded from secure storage */
unsigned char g_shared_key[SHARED_KEY_SIZE] = {
    0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
    0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
    0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
    0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88
};

/* Ed25519 keypair (generated once, stored securely) */
unsigned char g_public_key[crypto_sign_PUBLICKEYBYTES];
unsigned char g_secret_key[crypto_sign_SECRETKEYBYTES];

/**
 * encrypt_msg - Encrypt message using XChaCha20-Poly1305
 */
int encrypt_msg(unsigned char *msg, size_t msg_len,
                unsigned char *out, unsigned char *nonce) {
    if (!msg || !out || !nonce) {
        fprintf(stderr, "[crypto] Invalid parameters\n");
        return -1;
    }

    randombytes_buf(nonce, crypto_secretbox_NONCEBYTES);

    if (crypto_secretbox_easy(out, msg, msg_len, nonce, g_shared_key) != 0) {
        fprintf(stderr, "[crypto] Encryption failed\n");
        return -1;
    }

    return msg_len + crypto_secretbox_MACBYTES;
}

/**
 * decrypt_msg - Decrypt message
 */
int decrypt_msg(unsigned char *cipher, size_t cipher_len,
                unsigned char *nonce, unsigned char *out) {
    if (!cipher || !nonce || !out) {
        fprintf(stderr, "[crypto] Invalid parameters\n");
        return -1;
    }

    if (crypto_secretbox_open_easy(out, cipher, cipher_len,
                                   nonce, g_shared_key) != 0) {
        fprintf(stderr, "[crypto] Decryption failed - signature mismatch\n");
        return -1;
    }

    return cipher_len - crypto_secretbox_MACBYTES;
}

/**
 * sign_message - Sign message with Ed25519
 */
int sign_message(unsigned char *msg, size_t msg_len,
                 unsigned char *signature) {
    if (!msg || !signature) {
        fprintf(stderr, "[crypto] Invalid parameters\n");
        return -1;
    }

    unsigned long long sig_len;
    if (crypto_sign_detached(signature, &sig_len, msg, msg_len,
                            g_secret_key) != 0) {
        fprintf(stderr, "[crypto] Signing failed\n");
        return -1;
    }

    return 0;
}

/**
 * verify_signature - Verify Ed25519 signature
 */
int verify_signature(unsigned char *msg, size_t msg_len,
                     unsigned char *signature) {
    if (!msg || !signature) {
        fprintf(stderr, "[crypto] Invalid parameters\n");
        return -1;
    }

    if (crypto_sign_verify_detached(signature, msg, msg_len,
                                    g_public_key) != 0) {
        fprintf(stderr, "[crypto] Signature verification failed\n");
        return -1;
    }

    return 0;
}
