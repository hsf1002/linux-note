
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/acct.h>
#include <sys/stat.h>
#include <time.h>
#include "ugid.h"

#define TIME_BUF_SIZE 100


/**
 *  convert comp_t into long long
 */
static long long
comp_to_ll(comp_t ct)
{
    const int EXP_SIZE = 3;
    const int MANTISSA_SIZE = 13;
    const int MANTISSA_MASK = (1 << MANTISSA_SIZE) - 1;
    long long mantissa;
    long long exp;

    mantissa = ct & MANTISSA_MASK;
    exp = (ct >> MANTISSA_SIZE) & ((1 << EXP_SIZE) - 1);
    return mantissa << (exp * 3);
}

/**
 *   
// 必须使用root才能打开记账功能
skydeiMac:28-详述进程创建和程序执行 sky$ su
Password:
sh-3.2# touch pacct
sh-3.2# ./acct_on pacct 
process accouting enabled
sh-3.2# exit
exit
// 三个进程已经退出：acct_on、su、bash程序

// 一个sleep进程
skydeiMac:28-详述进程创建和程序执行 sky$  sleep 15 &
[1] 4237
// ulimit是shell内置命令，不会创建新进程
skydeiMac:28-详述进程创建和程序执行 sky$ ulimit -c unlimited
// 创建一个新进程
skydeiMac:28-详述进程创建和程序执行 sky$ cat
^\Quit: 3
// 以状态码2失败返回
skydeiMac:28-详述进程创建和程序执行 sky$ grep xxx badfile
grep: badfile: No such file or directory
[1]+  Done                    sleep 15
// echo是shell内置命令，不会创建新进程
skydeiMac:28-详述进程创建和程序执行 sky$ echo $?
2

// 查看记账记录
skydeiMac:28-详述进程创建和程序执行 sky$ ./acct_view pacct
* 
 */
int
main(int argc, char *argv[])    
{
    int acct_file;
    struct acct ac;
    ssize_t num_read;
    char *s;
    char time_buf[TIME_BUF_SIZE];
    struct tm *loc;
    time_t t;

    if (argc != 2 || 0 == strcmp(argv[1], "--help"))
        fprintf(stderr, "%s filepath\n", argv[0]);
    
    if (-1 == (acct_file = open(argv[0], O_RDONLY)))
        perror("open error");
    
    printf("command flag   term.user  start-time      CPU  elapsed\n");
    printf("               status     time                 time\n");

    while ((num_read = read(acct_file, &ac, sizeof(struct acct))) > 0)
    {
        if (num_read != sizeof(struct acct))
            perror("partial read");
        
        printf("%-8.8s  ", ac.ac_comm);
        printf("%c", (ac.ac_flag & AFORK) ? 'F' : '-');
        printf("%c", (ac.ac_flag & ASU) ? 'S' : '-');
        printf("%c", (ac.ac_flag & AXSIG) ? 'X' : '-');
        printf("%c", (ac.ac_flag & ACORE) ? 'C' : '-');

    #ifdef __linux__
        printf(" %#6lx  ", (unsigned long)ac.ac_exitcode);
    #else
//        printf(" %#6lx  ", (unsigned long)ac.ac_stat);
    #endif;

        s = username_from_id(ac.ac_uid);
        printf("%-8.8s ", (s == NULL) ? "???" : s);

        t = ac.ac_btime;
        if (NULL == (loc = localtime(&t)))
            printf("??? unknown time ???");
        else
        {
            strftime(time_buf, TIME_BUF_SIZE, "%Y-%m-%d %T ", loc);
            printf("%s ", time_buf);
        }

        printf("5.2f %7.2f ", (double)(comp_to_ll(ac.ac_utime) + comp_to_ll(ac.ac_stime)) / sysconf(_SC_CLK_TCK),
                                (double)(comp_to_ll(ac.ac_etime)) / sysconf(_SC_CLK_TCK));
        printf("\n");
    }
    
    if (-1 == num_read)
        perror("read error");
        
    exit(EXIT_SUCCESS);
}

