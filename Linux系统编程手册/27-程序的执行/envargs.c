#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>

extern char **environ;

/**
 *   
 * 显示参数列表和环境列表
 */
int
main(int argc, char *argv[])    
{
    char **ep;

    for (int i=0; i<argc; i++)
        printf("argv[%d] = %s\n", i, argv[i]);
    
    for (ep=environ; *ep!=NULL; ep++)
        printf("environ: %s\n", *ep);
    
    exit(EXIT_SUCCESS);
}

