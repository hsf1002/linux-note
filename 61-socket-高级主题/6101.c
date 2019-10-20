#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

/*
    从fd读取n个字节保存到buffer
*/
ssize_t 
readn(int fd, void *buffer, size_t n)
{
    ssize_t numRead;
    size_t totalRead;
    char *buf;

    buf = buffer;

    for (totalRead=0; totalRead<n;)
    {
printf("fd = %d, total = %d, n = %d \n", fd, totalRead, n);

numRead = read(fd, buf, n - totalRead);
printf("numRead = %d\n", numRead);
        // 文件尾
        if (0 == numRead)
        {
            return totalRead;
        }
        // 读取错误
        else if (-1 == numRead)
        {
            if (EINTR == errno)
                continue;
            else
                return -1;
        }
        printf("read, num = %d, buf = %s\n",numRead,  buf);
        // 读取成功
        totalRead += numRead;
        buf += numRead; 

        printf("read, buf = %s\n", buf);
    }

    return totalRead;
}

/*
    将buffer的n个字节的数据写入fd
*/
ssize_t 
writen(int fd, const void *buffer, size_t n)
{
    ssize_t numWritten;
    size_t totalWritten;
    const char *buf;

    buf = buffer;
    for (totalWritten=0; totalWritten<n;)
    {
        numWritten = write(fd, buf, n - totalWritten);

        if (numWritten <= 0)
        {
            if (numWritten == -1 && errno == EINTR)
                continue;
            else
                return -1;
        }

        totalWritten += numWritten;
        buf += numWritten;
    }

    return totalWritten;
}

/**
 * 
 */
int 
main(int argc, char **argv)
{
    const char *filename = "test.txt";
    int fd = -1;

    if (-1 == (fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO)))
    {
        perror("open error");
        return -1;
    }

    printf("fd = %d\n", fd);

    writen(fd, "hello world, this is sky from Mars.", 20);
 printf("fd = %d\n", fd);
    char buf[50];
    readn(fd, buf, 20);

    printf("\nbuf=%s \n", buf);

    close(fd);  
}
