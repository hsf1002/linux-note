#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>

#define IS_ADDR_STR_LEN 256

/**
 
 skydeiMac:61-socket-高级主题 sky$ ./sockname 55555 &
[1] 33118
skydeiMac:61-socket-高级主题 sky$ netstat -a |grep '55555'
tcp6       0      0  localhost.55555        localhost.58623        TIME_WAIT
tcp6       0      0  localhost.58623        localhost.55555        TIME_WAIT
[1]+  Segmentation fault: 11  ./sockname 55555
 * 
 */
int 
main(int argc, char **argv)
{
    int listenFd, acceptFd, connFd;
    socklen_t len;
    void *addr;
    char addrStr[IS_ADDR_STR_LEN];

    if (argc != 2 || strcmp(argv[1], "--help") == 0)
        fprintf(stderr, "%s service", argv[0]);
    
    if (-1 == (listenFd = inetListen(argv[1], 5, &len)))
        perror("inetListen error");

    if (-1 == (connFd = inetConnect(NULL, argv[1], SOCK_STREAM)))
        perror("inetConnect error");

    if (-1 == (acceptFd = accept(listenFd, NULL, NULL)))
        perror("acept error");

    if (NULL == (addr = malloc(len)))
        perror("malloc error");

    if (getsockname(connFd, addr, &len) == -1)
        perror("getsockname connFd error");
    else
        printf("connFd: %s \n", inetAddressStr(addr, len, addrStr, IS_ADDR_STR_LEN));
    
    if (getsockname(acceptFd, addr, &len) == -1)
        perror("getsockname acceptFd error");
    else
        printf("acceptFd: %s \n", inetAddressStr(addr, len, addrStr, IS_ADDR_STR_LEN));
    
    if (getpeername(connFd, addr, &len) == -1)
        perror("getpeername connFd error");
    else
        printf("connFd: %s \n", inetAddressStr(addr, len, addrStr, IS_ADDR_STR_LEN));
    
    if (getpeername(acceptFd, addr, &len) == -1)
        perror("getpeername acceptFd error");
    else
        printf("acceptFd: %s \n", inetAddressStr(addr, len, addrStr, IS_ADDR_STR_LEN));
    
    // give some time to run netstat
    sleep(30);
    exit(EXIT_SUCCESS);
}
