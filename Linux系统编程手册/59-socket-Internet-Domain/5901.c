#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>



/*
    根据给定的socket type创建一个socket，并将其连接到通过host和service指定的地址
*/
int 
inetConnect(const char *host, const char *service, int type)
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sfd, s;

    memset(&hints, 0x00, sizeof(struct addrinfo));
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    hints.ai_family = AF_UNSPEC;    // 允许IPv4和IPv6
    hints.ai_socktype = type;
    
    // 根据给定的host、service和type，返回一个地址列表
    if (0 != (s = getaddrinfo(host, service, &hints, &result)))
    {
        errno = ENOSYS;
        return -1;
    }

    // 轮询该地址列表
    for (rp=result; rp!=NULL; rp=rp->ai_next)
    {
        // 创建
        if (-1 == (sfd = (socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol))))
            continue;
        // 连接成功
        if (-1 != connect(sfd, rp->ai_addr, rp->ai_addrlen))
            break;
        // 连接失败，尝试下一个地址
        close(sfd);
    }

    // 释放
    freeaddrinfo(result);

    return(rp == NULL) ? -1 : sfd;
}

/*
    供inetListen和inetBind调用
*/
static int 
inetPassiveSocket(const char *service, int type, socklen_t *addrlen, bool doListen, int backlog)
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sfd, s, optval;

    memset(&hints, 0x00, sizeof(struct addrinfo));
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    hints.ai_family = AF_UNSPEC;    // 允许IPv4和IPv6
    hints.ai_socktype = type;
    hints.ai_flags = AI_PASSIVE;    // 返回一个监听socket的地址结构，此时host是NULL，result返回的socket地址结构的IP部分将包含一个通配IP地址（INADDR_ANY或IN6ADDR_ANY_INIT）；如果没有此标记，result返回的结构将能用于connect或sendto；如果host是NULL，返回的socket地址的IP部分将会被设置为回环IP地址（INADDR_LOOPBACK或IN6ADDR_LOOPBACK_INIT）
    
    // 根据给定的service和type，返回一个地址列表
    if (0 != (s = getaddrinfo(NULL, service, &hints, &result)))
    {
        errno = ENOSYS;
        return -1;
    }

    optval = 1;

    // 轮询该地址列表
    for (rp=result; rp!=NULL; rp=rp->ai_next)
    {
        // 创建
        if (-1 == (sfd = (socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol))))
            continue;
        // 是监听socket
        if (doListen)
        {
            // SO_REUSEADDR允许启动一个监听服务器并捆绑其众所周知端口，即使以前建立的将此端口用做他们的本地端口的连接仍存在。这通常是重启监听服务器时出现，若不设置此选项，则bind时将出错
            // 允许在同一端口上启动同一服务器的多个实例，只要每个实例捆绑一个不同的本地IP地址即可。对于TCP，我们根本不可能启动捆绑相同IP地址和相同端口号的多个服务器
            // 允许单个进程捆绑同一端口到多个套接口上，只要每个捆绑指定不同的本地IP地址即可。这一般不用于TCP服务器
            // 允许完全重复的捆绑：当一个IP地址和端口绑定到某个套接口上时，还允许此IP地址和端口捆绑到另一个套接口上。一般来说，这个特性仅在支持多播的系统上才有，而且只对UDP套接口而言（TCP不支持多播）
            if (-1 == (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))))
            {
                close(sfd);
                freeaddrinfo(result);
                return -1;
            }

        }
        // 绑定成功
        if (0 == bind(sfd, rp->ai_addr, rp->ai_addrlen))
            break;
        // 绑定失败，尝试下一个地址
        close(sfd);
    }

    if (rp != NULL && doListen)
    {
        // 监听
        if (-1 == listen(sfd, backlog))
        {
            freeaddrinfo(result);
            return -1;
        }
    }

    if (rp != NULL && addrlen != NULL)
        *addrlen = rp->ai_addrlen;

    // 释放
    freeaddrinfo(result);
    
    return(rp == NULL) ? -1 : sfd;
}

/**
 * 创建一个监听流socket，该socket会绑定到由service指定的TCP端口的通配IP地址上
 */
int 
inetListen(const char *service, int backlog, socklen_t *addrlen)
{
    return inetPassiveSocket(service, SOCK_STREAM, addrlen, true, backlog);
}

/**
 * 根据指定的type创建一个socket，并将其绑定到由service和type指定的端口的通配IP地址上
 */
int 
inetBind(const char *service, int type, socklen_t *addrlen)
{
    return inetPassiveSocket(service, type, addrlen, false, 0);
}

/*
    返回一个已null结尾的字符串，其包含了对应的主机名和端口号
*/
char *
inetAddressStr(const struct sockaddr *addr, socklen_t addrlen, char *addrStr, int addrStrLen)
{
    char host[NI_MAXHOST], service[NI_MAXSERV];

    if (0 == getnameinfo(addr, addrlen, host, NI_MAXHOST, service, NI_MAXSERV, NI_NUMERICSERV))
        snprintf(addrStr, addrStrLen, "(%s,%s)", host, service);
    else
        snprintf(addrStr, addrStrLen, "(?UNKNOWN)");
    
    addrStr[addrStrLen - 1] = '\0';
    return addrStr;
}
