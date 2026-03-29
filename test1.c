#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "license_client.h"
#include "verify_sav.h"

#define RESULT_BUF_SIZE 512

int main(int argc, char *argv[]) {
    int status;
    int cached_status = -1;
	char result[RESULT_BUF_SIZE];
	memset(result, 0, sizeof(result));

    if (verify_sav_get(&cached_status) == 0 && cached_status == 0) {
        printf("[app] Cached valid license found, skip remote verification\n");
        status = 0;
        strcpy(result, "Cached license OK");
    } else {
		if (argc < 2) {
			fprintf(stderr, "Usage: %s <activation_code> [device_id] [server_host] [server_port]\n", argv[0]);
			fprintf(stderr, "\nExamples:\n");
			fprintf(stderr, "  %s VALID_CODE\n", argv[0]);
			fprintf(stderr, "  %s VALID_CODE PC001 127.0.0.1 22345\n", argv[0]);
			fprintf(stderr, "\nTest Codes:\n");
			fprintf(stderr, "  VALID_CODE        - Never expires, unlimited devices\n");
			fprintf(stderr, "  CODE_ABC123       - Expires in 30 days, max 5 devices\n");
			fprintf(stderr, "  EXPIRED_CODE      - Already expired\n");
			return 1;
		}

		const char *activation_code = argv[1];
		const char *device_id = (argc > 2) ? argv[2] : "DEFAULT_DEVICE";
		const char *host = (argc > 3) ? argv[3] : "127.0.0.1";
		int port = (argc > 4) ? atoi(argv[4]) : 22345;

		printf("=== Commercial License Verification Tool v1.0 ===\n\n");
		printf("[app] Activation Code Verification\n");
		printf("[app] Code: %s\n", activation_code);
		printf("[app] Device: %s\n", device_id);
		printf("[app] Server: %s:%d\n\n", host, port);





		status = license_verify(host, port, activation_code, device_id,
								result, sizeof(result));
		if (status == 0) {
			verify_sav_set(0); // 保存成功状态
		}
    }

    printf("=== Verification Result ===\n");
    printf("[app] Status Code: %d\n", status);
    printf("[app] Message: %s\n", result);

    if (status == 0) {  /* RESPONSE_OK = 0 */
        printf("\n✓ License verification SUCCESSFUL\n");
        return 0;
    } else if (status == 1) {  /* RESPONSE_INVALID_CODE */
        printf("\n✗ Invalid activation code\n");
        return 1;
    } else if (status == 2) {  /* RESPONSE_EXPIRED */
        printf("\n✗ Activation code has expired\n");
        return 1;
    } else if (status == 3) {  /* RESPONSE_DEVICE_LIMIT */
        printf("\n✗ Device limit reached for this code\n");
        return 1;
    } else {  /* RESPONSE_SERVER_ERROR */
        printf("\n✗ Server error or connection failed\n");
        return 1;
    }

    return 0;
}
