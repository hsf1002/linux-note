#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <limits.h>
#include <time.h>
#include <sys/time.h>
#include <locale.h>

#define BUF_SIZE  1000


/**
 *   返回当前时间的字符串
 */
char *
curr_time(const char *format)
{
    static char buf[BUF_SIZE];
    time_t t;
    size_t s;
    struct tm *tm;

    t = time(NULL);

    if (NULL == (tm = localtime(&t)))
        perror("localtime error");
    
    s = strftime(buf, BUF_SIZE, (format != NULL) ? format : "%c", tm);
    return (0 == s) ? NULL : buf;
}

/**
 * 
skydeiMac:10-时间 sky$ ./a.out 
strftime of local time: Sunday, 10 November 2019, 08:41:52 CST 

 */
int
main(int argc, char *argv[])    
{
    char *p = NULL;

    if (NULL != (p = curr_time("%A, %d %B %Y, %H:%M:%S %Z")))
        printf("strftime of local time: %s\n", p);

    exit(EXIT_SUCCESS);
}

