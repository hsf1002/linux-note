#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <signal.h>
#include <netdb.h>

/* Bit-mask values for 'flags' argument of becomeDaemon() */
#define BD_NO_CHDIR           01    /* Don't chdir("/") */
#define BD_NO_CLOSE_FILES     02    /* Don't close all open files */
#define BD_NO_REOPEN_STD_FDS  04    /* Don't reopen stdin, stdout, andstderr to /dev/null */
#define BD_NO_UMASK0         010    /* Don't do a umask(0) */
#define BD_MAX_CLOSE  8192          /* Maximum file descriptors to close if sysconf(_SC_OPEN_MAX) is indeterminate */

#define SERVICE "echo"
#define BUFSIZE 500


/**
 * 设置为守护进程
 */
int                                     /* Returns 0 on success, -1 on error */
becomeDaemon(int flags)
{
    int maxfd, fd;

    switch (fork()) 
    {                   /* Become background process */
        case -1: 
            return -1;
        case 0:  
            break;                     /* Child falls through... */
        default: 
            _exit(EXIT_SUCCESS);       /* while parent terminates */
    }

    if (setsid() == -1)                 /* Become leader of new session */
        return -1;

    switch (fork()) 
    {                   /* Ensure we are not session leader */
        case -1: 
            return -1;
        case 0:  
            break;
        default: 
            _exit(EXIT_SUCCESS);
    }

    if (!(flags & BD_NO_UMASK0))
        umask(0);                       /* Clear file mode creation mask */

    if (!(flags & BD_NO_CHDIR))
        chdir("/");                     /* Change to root directory */

    if (!(flags & BD_NO_CLOSE_FILES)) 
    { /* Close all open files */
        maxfd = sysconf(_SC_OPEN_MAX);
        if (maxfd == -1)                /* Limit is indeterminate... */
            maxfd = BD_MAX_CLOSE;       /* so take a guess */

        for (fd = 0; fd < maxfd; fd++)
            close(fd);
    }

    if (!(flags & BD_NO_REOPEN_STD_FDS)) 
    {
        close(STDIN_FILENO);            /* Reopen standard fd's to /dev/null */

        fd = open("/dev/null", O_RDWR);

        if (fd != STDIN_FILENO)         /* 'fd' should be 0 */
            return -1;
        if (dup2(STDIN_FILENO, STDOUT_FILENO) != STDOUT_FILENO)
            return -1;
        if (dup2(STDIN_FILENO, STDERR_FILENO) != STDERR_FILENO)
            return -1;
    }

    return 0;
}


/**
 * 迭代型的UDP echo服务器
 */
int 
main(int argc, char **argv)
{
    int sfd;
    ssize_t numRead;
    socklen_t addrlen, len;
    struct sockaddr_storage claddr;
    char buf[BUFSIZ];
    char addrStr[BUFSIZ];

    if (becomeDaemon(0) == -1)
        perror("becomeDaemon error");
    if (-1 == (sfd = inetBind(SERVICE, SOCK_DGRAM, &addrlen)))
    {
        perror("create socket error");
        exit(EXIT_FAILURE);
    }

    for (;;)
    {
        len = sizeof(struct sockaddr_storage);
        if (-1 == (numRead = recvfrom(sfd, buf, BUFSIZ, 0, (struct sockaddr *)&claddr, &len)))
            perror("recvfrom error");
        if (numRead != sendto(sfd, buf, numRead, 0, (struct sockaddr *)&claddr, len))
            perror("sento error");
    }
    exit(EXIT_SUCCESS);
}
