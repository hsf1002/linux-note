#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>


static void             
usage_error(const char *progName, const char *msg)
{
    if (msg != NULL)
        fprintf(stderr, "%s", msg);
    fprintf(stderr, "Usage: %s [-cx] {-f pathname | -k key | -p} "
                            "[octal-perms]\n", progName);
    fprintf(stderr, "    -c           Use IPC_CREAT flag\n");
    fprintf(stderr, "    -x           Use IPC_EXCL flag\n");
    fprintf(stderr, "    -f pathname  Generate key using ftok()\n");
    fprintf(stderr, "    -k key       Use 'key' as key\n");
    fprintf(stderr, "    -p           Use IPC_PRIVATE key\n");

    exit(EXIT_FAILURE);
}


/**
 *   创建System V消息队列
 */
int
main(int argc, char *argv[])    
{
    int num_key_flag = 0;    // 统计-f,-k,-p选项
    int flag = 0, msgid = 0, opt = 0;
    unsigned int perms;
    long lkey;
    key_t key;

    while (-1 != (opt = getopt(argc, argv, "cf:k:px")))
    {
        switch (opt)
        {
            case 'c':
            {
                flag |= IPC_CREAT;
            }
            break;
            case 'f':
            {
                // 根据指定的文件生成key
                if (-1 == (key = ftok(optarg, 1)))
                {
                    perror("ftok error");
                    exit(EXIT_FAILURE);
                }
                num_key_flag++;
            }
            break;
            case 'k':
            {
                // 手动指定key
                if (1 != sscanf(optarg, "%li", &lkey))
                {
                    perror("-k option requires a numberic argument");
                    exit(EXIT_FAILURE);
                }
                key = lkey;
                num_key_flag++;
            }
            break;
            case 'p':
            {
                key = IPC_PRIVATE;
                num_key_flag++;
            }
            break;
            case 'x':
            {
                flag |= IPC_EXCL;
            }
            break;
            default:
            {
                usage_error(argv[0], "bad option\n");
            }
            break;
        }
    }

    // 必须携带-f、-k、-p一个参数，表示创建key的三种方式
    if (1 != num_key_flag)
        usage_error(argv[0], "exactly one of the options[-f][-k][-p] must be supplied\n");
    
    // 创建共享内存
    if (-1 == (msgid = msgget(key, flag | perms)))
    {
        perror("msgget error");
        exit(EXIT_FAILURE);
    }

    printf("svmsg create success, msgid = %d\n", msgid);

    exit(EXIT_SUCCESS);
}

