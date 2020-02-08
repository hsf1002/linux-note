//#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <stddef.h>
#include <signal.h>
#include <ctype.h>

#define MAX_LINE 100

static bool
mand_locking_enabled(int fd)
{
    struct stat sb;

    if (-1 == fstat(fd, &sb))
    {
        perror("fstat error");
    }
    return (sb.st_mode & S_ISGID) != 0 && (sb.st_mode & S_IXGRP) == 0;
}

static void
display_cmd_fmt()
{
    printf("\n    format: cmd lock start length [whence]\n\n");
    printf("       cmd:[g/s/w]\n");
    printf("       lock:[r/w/u]\n");
    printf("       start and length specify the range to lock\n");
    printf("       whence:[s/c/e]\n");
}

/**
 * 实验记录锁



terminal 1 进程A:
./i_fcntl_locking tfile
enter ? for help
PID=45920> s r 0 40   //  1. 0-40 加读锁成功
[PID=45920] got lock
PID=45920> g w 0 0    //  3. 检测是否可以加写锁？不能
[PID=45920] Denied by READ lock on 70:0 (held by PID 45921)
PID=45920> s w 0 0    //  4. 尝试加写锁，失败
[PID=45920] failed (incompatible lock)
PID=45920> w w 0 0    //  5. 以阻塞方式加锁
[PID=45920] got lock  //  9. 阻塞结束，加写锁成功


terminal 2 进程B:
./i_fcntl_locking tfile
enter ? for help
PID=45921> s r -30 0 e  // 2. 70-100 加读取成功
[PID=45921] got lock
PID=45921> g w 0 0      // 6. 检测是否可以加写锁？不能
[PID=45921] Denied by READ lock on 0:40 (held by PID 45920)
PID=45921> w w 0 0      // 7. 尝试加写锁，失败，出现死锁，内核选择让进程B接收EDEADLK错误
[PID=45921] failed (deadlock)
PID=45921> s u 0 0      // 8. 释放所有锁
[PID=45921] unlocked
 */
int main(int argc, char *argv[])
{
    int fd, cmd, num_read, status;
    char lock, cmd_ch, whence, line[MAX_LINE];
    struct flock fl;
    long long len, st;


    if (argc != 2 || strcmp(argv[1], "--help") == 0)
    {
        fprintf(stderr, "%s file\n", argv[0]);
    
        exit(EXIT_FAILURE);
    }

    // 打开文件
    if (-1 == (fd = open(argv[1], O_RDWR)))
    {
        perror("open error");
        exit(EXIT_FAILURE);
    }

    printf("enter ? for help\n");

    for (;;)
    {
        printf("PID=%ld> ", (long)getpid());
        fflush(stdout);

        if (NULL == fgets(line, MAX_LINE, stdin))
            exit(EXIT_SUCCESS);
        
        line[strlen(line) - 1] = '\0';

        if (*line == '\0')
            continue;
        
        if (line[0] == '?')
        {
            display_cmd_fmt();
            continue;
        }

        whence = 's';
        num_read = sscanf(line, "%c %c %lld %lld %c", &cmd_ch, &lock, &st, &len, &whence);

        fl.l_start = st;
        fl.l_len = len;

        // g: F_GETLK s: F_SETLK w: F_SETLKW     r: F_RDLCK w: F_WRLCK u: F_UNLCK    s: SEEK_SET c: SEEK_CUR e: SEEK_END
        if (num_read < 4 || strchr("gsw", cmd_ch) == NULL || strchr("rwu", lock) == NULL || strchr("sce", whence) == NULL)
        {
            printf("invalid command");
            continue;
        }

        cmd = (cmd_ch == 'g') ? F_GETLK : (cmd_ch == 's') ? F_SETLK : F_SETLKW;
        fl.l_type = (lock == 'r') ? F_RDLCK : (lock == 'w') ? F_WRLCK : F_UNLCK;
        fl.l_whence = (whence == 'c') ? SEEK_CUR : (whence == 'e') ? SEEK_END : SEEK_SET;

        status = fcntl(fd, cmd, &fl);

        if (cmd == F_GETLK)
        {
            if (-1 == status)
                perror("fcntl-F_GETLK error");
            else
            {
                if (fl.l_type == F_UNLCK)
                    printf("[PID=%ld] lock can not be placed\n", (long)getpid());
                else
                    printf("[PID=%ld] Denied by %s lock on %lld:%lld "
                            "(held by PID %ld)\n", (long) getpid(),
                            (fl.l_type == F_RDLCK) ? "READ" : "WRITE",
                            (long long) fl.l_start,
                            (long long) fl.l_len, (long) fl.l_pid);
            }
        }
        else
        {
            if (0 == status)
                printf("[PID=%ld] %s\n", (long)getpid(), (lock == 'u') ? "unlocked" : "got lock");
            else if (errno == EAGAIN || errno == EACCES)
                printf("[PID=%ld] failed (incompatible lock)\n", (long)getpid());
            else if (errno == EDEADLK)
                printf("[PID=%ld] failed (deadlock)\n", (long)getpid());
            else
                printf("fcntl F_SETLK error");
        }
    }

    exit(EXIT_SUCCESS);
}

