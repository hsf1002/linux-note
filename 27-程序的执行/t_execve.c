#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>



/**
 *   
 * 调用execve执行新程序
./t_execve ./envargs
argv[0] = ./envargs
argv[1] = hello world
argv[2] = goodbye
environ: GREET=sky
environ: BEST=lotus

*/
int
main(int argc, char *argv[])    
{
    char *arg_vec[5];
    char *env_vec[] = {"GREET=sky", "BEST=lotus", NULL};

    if (argc != 2 || 0 == strcmp(argv[1], "--help"))
        fprintf(stderr, "%s pathname\n", argv[0]);

    // 获取basename
    if (NULL == (arg_vec[0] = strrchr(argv[1], '/')))
        arg_vec[0]++;
    else
        arg_vec[0] = argv[1];
    
    arg_vec[1] = "hello world";
    arg_vec[2] = "goodbye";
    // 必须以NULL结束
    arg_vec[3] = NULL;

    execve(argv[1], arg_vec, env_vec);
    
    printf("execve error\n");
    // 走到这，说明出错
    exit(EXIT_FAILURE);
}

