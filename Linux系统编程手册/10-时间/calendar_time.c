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

#define SECOND_IN_ONE_YEAR (365.24219 * 24 * 60 * 60)

/**
 *   获取和转换日历时间

seconds since the Epoch(1970-01-01): 1573342093, (about 49.857 years) 
gettimeofday return 1573342093 seconds, 430223 microsecs 
Broken down by gmtime: 
 year=119, mon=10, day=9, hour=23, min=28, sec=13
Broken down by localtime: 
 year=119, mon=10, day=10, hour=7, min=28, sec=13
wday=0, yday=313, isdst=0

astime formats the gmtime value as: Sat Nov  9 23:28:13 2019
ctime formats the time value as:    Sun Nov 10 07:28:13 2019

mktime of gmtime value:     1573313293 secs
mktime of localtime value:  1573342093 secs
*/
int
main(int argc, char *argv[])    
{
    time_t t;
    struct tm *gmp, *locp;
    struct tm gm;
    struct tm loc;
    struct timeval tv;

    // 获取当前日历时间
    t = time(NULL);
    printf("seconds since the Epoch(1970-01-01): %ld,", (long)t);
    printf(" (about %6.3f years) \n", t / SECOND_IN_ONE_YEAR);

    // 获取当前日历时间，包含秒+微秒
    if (-1 == gettimeofday(&tv, NULL))
        perror("gettimeofday error");

    printf("gettimeofday return %ld seconds, %ld microsecs \n", (long)tv.tv_sec, (long)tv.tv_usec);

    // 将日历时间转换为分解时间
    if (NULL == (gmp = gmtime(&t)))
        perror("gmtime error");

    // 打印分解时间
    gm = *gmp;
    printf("Broken down by gmtime: \n");
    printf(" year=%d, mon=%d, day=%d, hour=%d, min=%d, sec=%d\n", \
        gm.tm_year, gm.tm_mon, gm.tm_mday, gm.tm_hour, gm.tm_min, gm.tm_sec);
    
    // 将日历时间转换为分解时间，考虑时区+夏令时
    if (NULL == (locp = localtime(&t)))
        perror("localtime error");

    // 打印分解时间
    loc = *locp;
    printf("Broken down by localtime: \n");
    printf(" year=%d, mon=%d, day=%d, hour=%d, min=%d, sec=%d\n", \
        loc.tm_year, loc.tm_mon, loc.tm_mday, loc.tm_hour, loc.tm_min, loc.tm_sec);
    printf("wday=%d, yday=%d, isdst=%d\n\n", loc.tm_wday, loc.tm_yday, loc.tm_isdst);

    // 分解时间转换为可打印格式，无法控制返回字符串格式，会自动添加换行符
    printf("astime formats the gmtime value as: %s", asctime(&gm));
    printf("ctime formats the time value as:    %s\n", ctime(&t));

    // 将分解时间转换为日历时间
    printf("mktime of gmtime value:     %ld secs\n", (long)mktime(&gm));
    printf("mktime of localtime value:  %ld secs\n", (long)mktime(&loc));

    exit(EXIT_SUCCESS);
}

