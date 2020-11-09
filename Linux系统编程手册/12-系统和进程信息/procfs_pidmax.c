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
        system("echo /proc/sys/kernel/pid_max now contains" "'cat /proc/sys/kernel/pid_max'");
    }

    exit(EXIT_SUCCESS);
}

