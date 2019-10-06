#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <signal.h>
#include <netdb.h>

#define INT_LEN  30
#define PORT_NUM "50000"
#define BACKLOG  50

#define ADDRSTRLEN  (NI_MAXHOST + NI_MAXSERV + 10)


#define GN_NONNEG       01      /* Value must be >= 0 */
#define GN_GT_0         02      /* Value must be > 0 */

                                /* By default, integers are decimal */
#define GN_ANY_BASE   0100      /* Can use any base - like strtol(3) */
#define GN_BASE_8     0200      /* Value is expressed in octal */
#define GN_BASE_16    0400      /* Value is expressed in hexadecimal */


/* Print a diagnostic message that contains a function name ('fname'),
   the value of a command-line argument ('arg'), the name of that
   command-line argument ('name'), and a diagnostic error message ('msg'). */

static void
gnFail(const char *fname, const char *msg, const char *arg, const char *name)
{
    fprintf(stderr, "%s error", fname);
    if (name != NULL)
        fprintf(stderr, " (in %s)", name);
    fprintf(stderr, ": %s\n", msg);
    if (arg != NULL && *arg != '\0')
        fprintf(stderr, "        offending text: %s\n", arg);

    exit(EXIT_FAILURE);
}
/* Convert a numeric command-line argument ('arg') into a long integer,
   returned as the function result. 'flags' is a bit mask of flags controlling
   how the conversion is done and what diagnostic checks are performed on the
   numeric result; see get_num.h for details.

   'fname' is the name of our caller, and 'name' is the name associated with
   the command-line argument 'arg'. 'fname' and 'name' are used to print a
   diagnostic message in case an error is detected when processing 'arg'. */

static long
getNum(const char *fname, const char *arg, int flags, const char *name)
{
    long res;
    char *endptr;
    int base;

    if (arg == NULL || *arg == '\0')
        gnFail(fname, "null or empty string", arg, name);

    base = (flags & GN_ANY_BASE) ? 0 : (flags & GN_BASE_8) ? 8 :
                        (flags & GN_BASE_16) ? 16 : 10;

    errno = 0;
    res = strtol(arg, &endptr, base);
    if (errno != 0)
        gnFail(fname, "strtol() failed", arg, name);

    if (*endptr != '\0')
        gnFail(fname, "nonnumeric characters", arg, name);

    if ((flags & GN_NONNEG) && res < 0)
        gnFail(fname, "negative value not allowed", arg, name);

    if ((flags & GN_GT_0) && res <= 0)
        gnFail(fname, "value must be > 0", arg, name);

    return res;
}
/* Convert a numeric command-line argument string to a long integer. See the
   comments for getNum() for a description of the arguments to this function. */

long
getLong(const char *arg, int flags, const char *name)
{
    return getNum("getLong", arg, flags, name);
}
/* Convert a numeric command-line argument string to an integer. See the
   comments for getNum() for a description of the arguments to this function. */

int
getInt(const char *arg, int flags, const char *name)
{
    long res;

    res = getNum("getInt", arg, flags, name);

    if (res > INT_MAX || res < INT_MIN)
        gnFail("getInt", "integer out of range", arg, name);

    return (int) res;
}

/*
    一次读取一行
 */
ssize_t
readLine(int fd, void *buffer, size_t n)
{
    ssize_t numRead;
    size_t toRead;
    char *buf;
    char ch;

    if (n <= 0 || buffer == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    buf = buffer;
    toRead = 0;

    for (;;)
    {
        // 一次读取一个字节
        numRead = read(fd, &ch, 1);

        // 读取失败
        if (numRead == -1)
        {
            if (errno == EINTR)
                continue;
            else
                return -1;
        }
        // 遇到EOF
        else if (numRead == 0)
        {
            // 没有读取到数据，之间返回
            if (toRead == 0)
                return 0;
            // 读取到了数据，添加'\0'
            else
                break;
        }
        else
        {
            // 没有读取完毕，继续读取
            if (toRead < n - 1)
            {
                toRead++;
                *buf++ = ch;
            }
            // 如果遇到换行符，停止
            if (ch == '\n')
                break;
        }
    }

    *buf = '\0';
    return toRead;
}

/**
 * Internet domain流socket服务端
 */
int 
main(int argc, char **argv)
{
    uint32_t seqNum;
    char reqLenStr[INT_LEN];
    char seqNumStr[INT_LEN];
    struct sockaddr_storage claddr;
    int lfd, cfd, optval, reqLen;
    socklen_t addrlen;
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    char addrStr[ADDRSTRLEN];
    char host[NI_MAXHOST];
    char service[NI_MAXSERV];

    if (argc > 1 && strcmp(argv[1], "--help") == 0)
        fprintf(stderr, "%s [init-seq-num] \n", argv[0]);
    
    seqNum = (argc > 1) ? getInt(argv[1], 0, "init-seq-num") : 0;

    // 忽略SIGPIPE信号，防止服务器在尝试向一个对端已经被关闭的socket写入数据时收到此信号
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
        perror("signal error");

    memset(&hints, 0x00, sizeof(struct addrinfo));
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;    // IPv4 & IPv6
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

    if (0 != getaddrinfo(NULL, PORT_NUM, &hints, &result))
        perror("getaddrinfo error"); 

    optval = 1;

    // 轮询该地址列表
    for (rp=result; rp!=NULL; rp=rp->ai_next)
    {
        // 创建
        if (-1 == (lfd = (socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol))))
            continue;

        // SO_REUSEADDR允许启动一个监听服务器并捆绑其众所周知端口，即使以前建立的将此端口用做他们的本地端口的连接仍存在。这通常是重启监听服务器时出现，若不设置此选项，则bind时将出错
        // 允许在同一端口上启动同一服务器的多个实例，只要每个实例捆绑一个不同的本地IP地址即可。对于TCP，我们根本不可能启动捆绑相同IP地址和相同端口号的多个服务器
        // 允许单个进程捆绑同一端口到多个套接口上，只要每个捆绑指定不同的本地IP地址即可。这一般不用于TCP服务器
        // 允许完全重复的捆绑：当一个IP地址和端口绑定到某个套接口上时，还允许此IP地址和端口捆绑到另一个套接口上。一般来说，这个特性仅在支持多播的系统上才有，而且只对UDP套接口而言（TCP不支持多播）
        if (-1 == (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))))
        {
            close(lfd);
            freeaddrinfo(result);
            return -1;
        }
        
        // 绑定成功
        if (0 == bind(lfd, rp->ai_addr, rp->ai_addrlen))
            break;
        // 绑定失败，尝试下一个地址
        close(lfd);
    }

    if (rp == NULL)
    {
        perror("bind error");
        return -1;
    }

    // 监听
    if (-1 == listen(lfd, BACKLOG))
    {
        freeaddrinfo(result);
        return -1;
    }
    
    // 释放
    freeaddrinfo(result);

    // 循环发送，接收数据
    for(;;)
    {
        addrlen = sizeof(struct sockaddr_storage);

        if (-1 == (cfd = accept(lfd, (struct sockaddr *)&claddr, &addrlen)))
            continue;
        
        // 获取对端主机号和服务名
        if (0 == getnameinfo((struct sockaddr*)&claddr, addrlen, host, NI_MAXHOST, service, NI_MAXSERV, 0))
            snprintf(addrStr, ADDRSTRLEN, "(%s, %s)", host, service);
        else
            snprintf(addrStr, ADDRSTRLEN, "(?unknown?)");
        
        // 从客户端接收数据
        if (readLine(cfd, reqLenStr, INT_LEN) <= 0)
        {
            close(cfd);
            continue;
        }

        if ((reqLen = atoi(reqLenStr)) <= 0)
        {
            close(cfd);
            continue;
        }

        snprintf(seqNumStr, INT_LEN, "%d\n", seqNum);
        // 把序列号发送数据到客户端
        if (write(cfd, &seqNumStr, strlen(seqNumStr)) != strlen(seqNumStr))
            perror("write error");
        // 更新序列号，累加而已
        seqNum += reqLen;

        if (-1 == close(cfd))
            perror("close error");
    }
}
