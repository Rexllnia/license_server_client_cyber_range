#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <time.h>
#include "common.h"
#include "verify_sav.h"

#define DEFAULT_PORT 22345
#define DEFAULT_HOST "fe80::7892:b6bf:4b:e797"
#define BUF_SIZE 1024

int license_verify(const char *host, int port,
                   const char *code, const char *device_id,
                   char *result, size_t result_size) {
    if (!host || !code || !device_id || !result || result_size == 0) {
        fprintf(stderr, "[license_client] Invalid parameters\n");
        return RESPONSE_SERVER_ERROR;
    }

    int sock = socket(AF_INET6, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("[license_client] socket");
        return RESPONSE_SERVER_ERROR;
    }

    struct sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port);

    if (inet_pton(AF_INET6, host, &addr.sin6_addr) <= 0) {
        fprintf(stderr, "[license_client] Invalid host: %s\n", host);
        close(sock);
        return RESPONSE_SERVER_ERROR;
    }

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("[license_client] connect");
        close(sock);
        return RESPONSE_SERVER_ERROR;
    }

    printf("[license_client] Connected to %s:%d\n", host, port);

    char request[BUF_SIZE];
    snprintf(request, sizeof(request), "%s\n%s", code, device_id);

    ssize_t sent = write(sock, request, strlen(request));
    if (sent < 0) {
        perror("[license_client] write");
        close(sock);
        return RESPONSE_SERVER_ERROR;
    }

    printf("[license_client] Sent activation code: %s\n", code);

    char recv_buf[BUF_SIZE];
    memset(recv_buf, 0, sizeof(recv_buf));
    ssize_t received = read(sock, recv_buf, sizeof(recv_buf) - 1);
    close(sock);

    if (received < 0) {
        perror("[license_client] read");
        return RESPONSE_SERVER_ERROR;
    }
    if (received == 0) {
        fprintf(stderr, "[license_client] Server closed connection\n");
        return RESPONSE_SERVER_ERROR;
    }

    recv_buf[received] = '\0';
    printf("[license_client] Server response: %s\n", recv_buf);

    int status_code = RESPONSE_SERVER_ERROR;
    char message[BUF_SIZE] = {0};
    if (sscanf(recv_buf, "[%d] %[^\n]", &status_code, message) >= 2) {
    } else {
        strncpy(message, recv_buf, sizeof(message) - 1);
    }

    strncpy(result, message, result_size - 1);
    result[result_size - 1] = '\0';
    return status_code;
}

int license_verify_simple(const char *code, const char *device_id,
                          char *result, size_t result_size) {
    return license_verify(DEFAULT_HOST, DEFAULT_PORT,
                          code, device_id, result, result_size);
}

int license_check(const char *code, const char *device_id) {
    int cached_status;
    if (verify_sav_get(&cached_status) == 0 && cached_status == 0) {
        printf("[license_client] Cache hit (verify_sav), status=%d\n", cached_status);
        return 1;
    }

    char result[BUF_SIZE];
    int remote_status = license_verify_simple(code, device_id, result, sizeof(result));
    if (remote_status == RESPONSE_OK) {
        verify_sav_set(0);
        return 1;
    }
    return 0;
}
