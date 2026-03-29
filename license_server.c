#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <time.h>
#include "common.h"
#include "activation_db.h"

#define DEFAULT_PORT 22345
#define BACKLOG 5

static volatile sig_atomic_t stop_flag = 0;

static void handle_signal(int sig) {
    (void)sig;
    stop_flag = 1;
}

/**
 * Process activation request and generate response
 */
void handle_activation_request(const ActivationRequest *req,
                               ActivationResponse *resp) {
    if (!req || !resp) {
        resp->status_code = RESPONSE_SERVER_ERROR;
        snprintf(resp->message, sizeof(resp->message), "Invalid request");
        return;
    }

    /* Extract device ID (remove leading/trailing whitespace) */
    char device_id[MAX_DEVICE_ID_LEN];
    strncpy(device_id, req->device_id, sizeof(device_id) - 1);
    device_id[sizeof(device_id) - 1] = '\0';

    /* Remove trailing newline/whitespace */
    size_t len = strlen(device_id);
    while (len > 0 && (device_id[len-1] == '\n' || device_id[len-1] == ' ')) {
        device_id[len-1] = '\0';
        len--;
    }

    printf("[license_server] Request - Code: %s, Device: %s\n",
           req->code, device_id);

    /* Validate activation code */
    int status = validate_code(req->code, device_id);
    resp->status_code = status;
    resp->timestamp = time(NULL);

    /* Generate response message */
    if (status == RESPONSE_OK) {
        int remaining_days = get_remaining_days(req->code);
        if (remaining_days == -1) {
            snprintf(resp->message, sizeof(resp->message),
                    "License activated successfully! No expiration date.");
        } else {
            snprintf(resp->message, sizeof(resp->message),
                    "License activated successfully! Valid for %d more days.",
                    remaining_days);
        }
    } else if (status == RESPONSE_INVALID_CODE) {
        snprintf(resp->message, sizeof(resp->message), "Invalid activation code");
    } else if (status == RESPONSE_EXPIRED) {
        snprintf(resp->message, sizeof(resp->message), "Activation code has expired");
    } else if (status == RESPONSE_DEVICE_LIMIT) {
        snprintf(resp->message, sizeof(resp->message), "Max device limit reached for this code");
    } else {
        snprintf(resp->message, sizeof(resp->message), "Server error");
    }

    printf("[license_server] Response - Status: %d, Message: %s\n",
           status, resp->message);

    /* Sign response */
    sign_message((unsigned char *)resp->message, strlen(resp->message),
                 resp->signature);
}

/**
 * Handle client connection
 */
void handle_client(int client_fd, const char *client_ip, int client_port) {
    printf("[license_server] Client connected: %s:%d\n", client_ip, client_port);

    ActivationRequest req;
    ActivationResponse resp;

    /* Read request */
    unsigned char enc_buf[1024];
    ssize_t n = read(client_fd, enc_buf, sizeof(enc_buf));
    if (n <= 0) {
        fprintf(stderr, "[license_server] Read failed\n");
        close(client_fd);
        return;
    }

    printf("[license_server] Received %ld bytes\n", n);

    /* In simple mode, we expect plain text code + device_id */
    /* Format: "CODE\nDEVICE_ID" */
    char req_buf[1024];
    ssize_t copy_size = (n < (ssize_t)(sizeof(req_buf) - 1)) ? n : sizeof(req_buf) - 1;
    memcpy(req_buf, enc_buf, copy_size);
    req_buf[copy_size] = '\0';

    /* Parse request */
    char *newline = strchr(req_buf, '\n');
    if (newline) {
        *newline = '\0';
        strncpy(req.code, req_buf, sizeof(req.code) - 1);
        strncpy(req.device_id, newline + 1, sizeof(req.device_id) - 1);
    } else {
        strncpy(req.code, req_buf, sizeof(req.code) - 1);
        strcpy(req.device_id, "unknown");
    }
    req.code[sizeof(req.code) - 1] = '\0';
    req.device_id[sizeof(req.device_id) - 1] = '\0';

    req.timestamp = time(NULL);
    req.version = PROTOCOL_VERSION;
    req.msg_type = REQUEST_VERIFY;

    /* Process request */
    memset(&resp, 0, sizeof(resp));
    handle_activation_request(&req, &resp);

    /* Send response */
    ssize_t resp_len = snprintf((char *)enc_buf, sizeof(enc_buf),
                               "[%d] %s",
                               resp.status_code, resp.message);

    if (write(client_fd, enc_buf, resp_len) != resp_len) {
        perror("[license_server] write");
    }

    close(client_fd);
    printf("[license_server] Client disconnected: %s:%d\n", client_ip, client_port);
}

int main(int argc, char *argv[]) {
    int port = (argc > 1) ? atoi(argv[1]) : DEFAULT_PORT;

    /* Initialize sodium library */
    if (sodium_init() < 0) {
        fprintf(stderr, "[license_server] Failed to initialize sodium\n");
        return 1;
    }

    /* Initialize activation database */
    init_activation_db();

    /* Create server socket */
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(server_fd);
        return 1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, BACKLOG) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    printf("[license_server] Listening on 0.0.0.0:%d\n", port);
    printf("[license_server] License server started (v%d)\n", PROTOCOL_VERSION);

    while (!stop_flag) {
        struct sockaddr_in client;
        socklen_t client_len = sizeof(client);
        int client_fd = accept(server_fd, (struct sockaddr *)&client, &client_len);
        if (client_fd < 0) {
            if (errno == EINTR && stop_flag) {
                break;
            }
            perror("accept");
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client.sin_addr, client_ip, sizeof(client_ip));
        int client_port = ntohs(client.sin_port);

        handle_client(client_fd, client_ip, client_port);
    }

    printf("[license_server] Shutting down\n");
    close(server_fd);
    return 0;
}
