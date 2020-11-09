#define _XOPEN_SOURCE

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
 * 获取和转换日历时间

skydeiMac:10-时间 sky$ ./a.out "2019-02-01 21:39:46" "%F %T"
calendar time (seconds since Epoch): 1549028386
strptime yields: 21:39:46 星期五, 01 二月 2019 CST
skydeiMac:10-时间 sky$ ./a.out "2019-02-01 21:39:46" "%F %T" "%x %X"
calendar time (seconds since Epoch): 1549028386
strptime yields: 2019/02/01 21时39分46秒

*/
int
main(int argc, char *argv[])    
{
    struct tm tm;
    char buf[BUF_SIZE];
    char *ofmt;

    if (argc < 3 || 0 == strcmp(argv[1], "--help"))
        fprintf(stderr, "%s input-date-time in-format[out-format]\n", argv[0]);
    
    // 为所有内容设置一个默认值
    if (NULL == setlocale(LC_ALL, ""))
        perror("setlocale error");
    
    memset(&tm, 0x00, sizeof(struct tm));

    // 命令行参数接收日期和时间，将其转换为分解时间
    if (NULL == (strptime(argv[1], argv[2], &tm)))
        perror("strptime error");

    tm.tm_isdst = -1;
    printf("calendar time (seconds since Epoch): %ld\n", (long)mktime(&tm));

    // 输出格式有命令行第三个参数指定，否则设置一个默认值
    ofmt = (argc > 3) ? argv[3] : "%H:%M:%S %A, %d %B %Y %Z";

    // 将分解时间转换为格式化字符串输出
    if (0 == strftime(buf, BUF_SIZE, ofmt, &tm))
        perror("strftime error again");

    printf("strptime yields: %s\n", buf);

    exit(EXIT_SUCCESS);
}

