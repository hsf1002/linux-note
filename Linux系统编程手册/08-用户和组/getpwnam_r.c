#include <sys/types.h>  /* Type definitions used by many programs */
#include <stdio.h>      /* Standard I/O functions */
#include <stdlib.h>     /* Prototypes of commonly used library functions,
                           plus EXIT_SUCCESS and EXIT_FAILURE constants */
#include <unistd.h>     /* Prototypes for many system calls */
#include <errno.h>      /* Declares errno and defines error constants */
#include <string.h>     /* Commonly used string-handling functions */
#include <stdbool.h>    /* 'bool' type plus 'true' and 'false' constants */
#include <pwd.h>

/**
 *  根据用户名查找 
 *  cc getpwnam_r.c -o getpwnam_r
 * 

 ./getpwnam_r hsf1002
Not found

./getpwnam_r hefeng
real_name = hefeng,,,, uid = 1000, gid = 1000, dir = /home/hefeng, shell = /bin/bash 

cat /etc/passwd |grep hefeng
hefeng:x:1000:1000:hefeng,,,:/home/hefeng:/bin/bash


*/

int
main(int argc, char *argv[])
{
    if (argc != 2 || strcmp(argv[1], "--help") == 0)
        printf("%s username\n", argv[0]);

    size_t bufSize = sysconf(_SC_GETPW_R_SIZE_MAX);
    char *buf = malloc(bufSize);
    if (buf == NULL)
        printf("malloc %zu error", bufSize);

    struct passwd *result;
    struct passwd pwd;

    int s = getpwnam_r(argv[1], &pwd, buf, bufSize, &result);
    if (s != 0)
        printf("getpwnam_r");

    if (result != NULL)
        printf("real_name = %s, uid = %d, gid = %d, dir = %s, shell = %s \n", pwd.pw_gecos, pwd.pw_uid, pwd.pw_gid, pwd.pw_dir, pwd.pw_shell);
    else
        printf("Not found\n");

    exit(EXIT_SUCCESS);
}