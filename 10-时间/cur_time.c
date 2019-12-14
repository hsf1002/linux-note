#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include "cur_time.h"

/**
 * 
 *  按照格式化要求显示时间格式
 */
char *
curr_time(const char *format)
{
    static char buf[BUFSIZ];  
    time_t t;
    size_t s;
    struct tm *tm;

    t = time(NULL);
    tm = localtime(&t);

    if (tm == NULL)
        return NULL;

    s = strftime(buf, BUFSIZ, (format != NULL) ? format : "%c", tm);

    return (s == 0) ? NULL : buf;
}
