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
 * 使用execle将程序参数指定为列表

./t_execle ./envargs
argv[0] = ./envargs
argv[1] = hello world
environ: GREET=sky
environ: BEST=lotus
skydeiMac:27-程序的
*/
int
main(int argc, char *argv[])    
{
    char *env_vec[] = {"GREET=sky", "BEST=lotus", NULL};
    char *filename;

    if (argc != 2 || 0 == strcmp(argv[1], "--help"))
        fprintf(stderr, "%s pathname\n", argv[0]);

    // 获取basename
    if (NULL == (filename = strrchr(argv[1], '/')))
        filename++;
    else
        filename = argv[1];
    
    execle(argv[1], filename, "hello world", (char *)NULL, env_vec);

    perror("execle error");
    
    exit(EXIT_SUCCESS);
}

