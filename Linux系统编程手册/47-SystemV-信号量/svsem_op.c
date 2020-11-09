//#define _GNU_SOURCE
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
#include <sys/sem.h>
#include <stddef.h>
#include <signal.h>
#include <ctype.h>
#include "get_num.h"


// 选项参数的个数，也是信号量的个数
#define MAX_SEMOPS 1000


static void
usage_error(const char *progName)
{
    fprintf(stderr, "Usage: %s semid op[,op...] ...\n\n", progName);
    fprintf(stderr, "'op' is either: <sem#>{+|-}<value>[n][u]\n");
    fprintf(stderr, "            or: <sem#>=0[n]\n");
    fprintf(stderr, "       \"n\" means include IPC_NOWAIT in 'op'\n");
    fprintf(stderr, "       \"u\" means include SEM_UNDO in 'op'\n\n");
    fprintf(stderr, "The operations in each argument are "
                    "performed in a single semop() call\n\n");
    fprintf(stderr, "e.g.: %s 12345 0+1,1-2un\n", progName);
    fprintf(stderr, "      %s 12345 0=0n 1+1,2-1u 1=0\n", progName);
    exit(EXIT_FAILURE);
}

static int
parse_ops(char arg, struct sembuf sops[])
{
    char *comma, *sign, *remaining, *flags;
    int num_opts;


    // 循环解析每个选项参数
    for (num_opts=0, remaining=arg; ; num_opts++)
    {
        if (num_opts >= MAX_SEMOPS)
        {
            perror("too many operations error");
            exit(EXIT_FAILURE);
        }

        // 参数为空
        if (*remaining == '\0')
        {
            perror("trailing comma or empty argument error");
            exit(EXIT_FAILURE);
        }

        // 强制转换后应该为数字
        if (!isdigit((unsigned char)*remaining))
        {
            perror("expected initial digit error");
            exit(EXIT_FAILURE);
        }

        // remaining为0=1或1-1之类，转为long后即为信号量在集合中索引如0或1
        sops[num_opts].sem_num = strtol(remaining, &sign, 10);
        // sign是索引之后的字符串如+-=
        if (*sign == '\0' || strchr("+-=", *sign) == NULL)
        {
            perror("expected [+-=] error");
            exit(EXIT_FAILURE);
        }

        // sign之后的字符表示信号量的值，应该是数字
        if (!isdigit((unsigned char)*(sign + 1)))
        {
            fprintf(stderr, "expected digit after %c in %s", *sign, arg);
            exit(EXIT_FAILURE);
        }

        // 给信号量赋值
        sops[num_opts].sem_op = strtol(sign + 1, &flags, 10);
        
        // -表示取反
        if (*sign == '-')
            sops[num_opts].sem_op = -sops[num_opts].sem_op;
        // =只能赋值0
        else if (*sign == '=')
            if (sops[num_opts].sem_op != 0)
                perror("expected 0");
        
        sops[num_opts].sem_flg = 0;

        for (;; flags++)
        {
            if (*flags == 'n')
                sops[num_opts].sem_flg |= IPC_NOWAIT;
            else if (*flags == 'u')
                sops[num_opts].sem_flg |= SEM_UNDO;
            else
                break;
        }    

        // 每一个选项参数结束后必须跟逗号，否则表示结束
        if (*flags != ',' && *flags != '\0')
        {
            fprintf(stderr, "bad trailing character (%c) in %s", *flags, arg);
            exit(EXIT_FAILURE);
        }

        // 获取下一个reamining，选项参数
        if (NULL == (comma = strchr(remaining, ',')))
            break;
        else
            remaining = comma + 1;
    }

    return num_opts + 1;
}

/**
 *   使用semop执行 System V 信号量 操作
 * 

 */
int
main(int argc, char *argv[])    
{
    struct sembuf buf[MAX_SEMOPS];
    int num_bufs;

    if (argc < 2 || 0 == strcmp(argv[1], "--help"))
    {
        fprintf(stderr, "%s [val1][val2][val3]... \n", argv[0]);
        exit(EXIT_FAILURE);
    }

    for (int i=2; argv[i] != NULL; i++)
    {
        num_bufs = parse_ops(argv[i], buf);

        printf("%5ld, about to semop() [%s]\n", (long)getpid(), argv[i]);

        if (-1 == semop(getInt(argv[1], 0, "semid"), buf, num_bufs))
        {
            fprintf(stderr, "semop (PID=%ld) error\n", (long)getpid());
            exit(EXIT_FAILURE);
        }

        printf("5ld, semop() completed [%s]\n", (long)getpid(), argv[i]);
    }
        
    exit(EXIT_SUCCESS);
}

