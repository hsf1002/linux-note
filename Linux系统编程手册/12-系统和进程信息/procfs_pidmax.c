#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>

#define MAX_LINE  100


/**
 *   访问/proc/sys/kernel/pid_max文件
 * 
 * cc procfs_pidmax.c -o procfs_pidmax
 * 
./procfs_pidmax 
4194304
./procfs_pidmax 4194300
open pid_max error: Permission denied
read pid_max error: Bad file descriptor
write pid_max error: Bad file descriptor
/proc/sys/kernel/pid_max now containscat /proc/sys/kernel/pid_max
old value: 

实际无权限修改
sudo echo 4194300 > /proc/sys/kernel/pid_max
bash: /proc/sys/kernel/pid_max: 权限不够
ll /proc/sys/kernel/pid_max
-rw-r--r-- 1 root root 0 2月  19 17:05 /proc/sys/kernel/pid_max

 */
int
main(int argc, char *argv[])    
{
    int fd;
    char line[MAX_LINE];
    ssize_t n;

    if (-1 == (fd = open("/proc/sys/kernel/pid_max", (argc > 1) ? O_RDWR : O_RDONLY)))
        perror("open pid_max error");

    if (-1 == (n = read(fd, line, MAX_LINE)))
        perror("read pid_max error");
    
    if (argc > 1)
        printf("old value: ");
    
    printf("%.*s", (int)n, line);

    if (argc > 1)
    {
        if (write(fd, argv[1], strlen(argv[1])) != strlen(argv[1]))
            perror("write pid_max error");
        system("echo /proc/sys/kernel/pid_max now contains "
               "`cat /proc/sys/kernel/pid_max`");
    }

    exit(EXIT_SUCCESS);
}

