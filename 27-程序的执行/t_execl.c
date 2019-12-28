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
 * 使用execl将调用者的环境传递给新程序

skydeiMac:27-程序的执行 sky$ echo $USER $SHELL
sky /bin/bash
skydeiMac:27-程序的执行 sky$ ./t_execl
initial value of USER = sky
lolita
*/
int
main(int argc, char *argv[])    
{
    printf("initial value of USER = %s\n", getenv("USER"));

    if (0 != putenv("USER=lolita"))
        perror("putenv error");
    
    execl("/usr/bin/printenv", "printenv", "USER", "SHELL", (char *)NULL);

    perror("execl error");
    
    exit(EXIT_SUCCESS);
}

