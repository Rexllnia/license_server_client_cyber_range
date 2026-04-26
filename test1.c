#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "license_client.h"
#include "verify_sav.h"

#define RESULT_BUF_SIZE 512
typedef void (*daemon_func_t)(void *arg);
int create_daemon(daemon_func_t func,void *arg);
void bussiness_logic(void *arg);

struct verify_arguments
{
    int argc;
    const char *activation_code;
    const char *host;
    int port;
    const char *device_id;
};

void verify_logic(void *arg) {
    int status;
    int cached_status = -1;
	char result[RESULT_BUF_SIZE];
    struct verify_arguments *verify_arguments = (struct verify_arguments *)arg;
    const char *activation_code = verify_arguments->activation_code;
    const char *host = verify_arguments->host;
    int port = verify_arguments->port;
    const char *device_id = verify_arguments->device_id;

	memset(result, 0, sizeof(result));
    
    printf("verify_logic \n");

    if (verify_sav_get(&cached_status) == 0 && cached_status == 0) {
        printf("[app] Cached valid license found, skip remote verification\n");
        status = 0;
        strcpy(result, "Cached license OK");
    } else {
        if (verify_arguments->argc < 2) {
                    fprintf(stderr, "Usage: %d <activation_code> [device_id] [server_host] [server_port]\n", verify_arguments->port);
                    fprintf(stderr, "\nExamples:\n");
                    fprintf(stderr, "  %s VALID_CODE\n", verify_arguments->activation_code);
                    fprintf(stderr, "  %s VALID_CODE PC001 ::1 22345\n", verify_arguments->activation_code);
                    fprintf(stderr, "\nTest Codes:\n");
                    fprintf(stderr, "  VALID_CODE        - Never expires, unlimited devices\n");
                    fprintf(stderr, "  CODE_ABC123       - Expires in 30 days, max 5 devices\n");
                    fprintf(stderr, "  EXPIRED_CODE      - Already expired\n");
                    return;
                }



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
        create_daemon(bussiness_logic, NULL);
    } else if (status == 1) {  /* RESPONSE_INVALID_CODE */
        printf("\n✗ Invalid activation code\n");
    } else if (status == 2) {  /* RESPONSE_EXPIRED */
        printf("\n✗ Activation code has expired\n");
    } else if (status == 3) {  /* RESPONSE_DEVICE_LIMIT */
        printf("\n✗ Device limit reached for this code\n");
    } else {  /* RESPONSE_SERVER_ERROR */
        printf("\n✗ Server error or connection failed\n");
    }
}
void bussiness_logic(void *arg) 
{
    // 这里可以放置实际的业务逻辑代码
    while (1) {
        printf("[daemon] Running business logic...\n");
        sleep(5);  // 模拟业务逻辑的周期性执行
    }
}
int create_daemon(daemon_func_t func,void *arg) 
{
    pid_t pid = fork();
    if (pid < 0) {
        // fork失败
        perror("fork failed");
        exit(1);
    } 

    if (pid > 0) {
        exit(0);  // 父进程退出
    } else {
        func(arg);
        exit(0);  // 子进程退出
    }
}


int main(int argc, char *argv[]) {
   struct verify_arguments verify_arguments;
   verify_arguments.argc = argc;
   verify_arguments.activation_code = (argc > 1) ? argv[1] : NULL;
   verify_arguments.host = (argc > 2) ? argv[2] : "::1";
   verify_arguments.port = (argc > 3) ? atoi(argv[3]) : 22345;
   verify_arguments.device_id = (argc > 4) ? argv[4] : "DEFAULT_DEVICE";
   create_daemon(verify_logic,&verify_arguments);
return 0;   
}
