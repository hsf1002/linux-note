#include <stdio.h>
#include <sys/uio.h>
#include <fcntl.h>

/**
 * 
 * cc readv.c -o readv
 * 
 * ./readv 
rsize = 15
wsize = 15

*/

int main(void)
{
    int fd;
    char buf1[5];
    char buf2[10];
    struct iovec iov[2];
    iov[0].iov_base = buf1;
    iov[0].iov_len = 5;
    iov[1].iov_base = buf2;
    iov[1].iov_len = 10;

    if ((fd = open("a.txt", O_RDWR)) < 0)
    {
        perror("open error");
        return -1;
    }
    int rsize = readv(fd, iov, 2); // 第三个参数是iov的个数
    printf("rsize = %d\n",rsize);

    close(fd);

    if ((fd = open("b.txt", O_RDWR|O_CREAT, S_IRUSR|S_IWUSR)) < 0)
    {
        perror("open error");
        return -1;
    }

    int wsize = writev(fd,iov,2); // 第三个参数是iov的个数
    printf("wsize = %d\n",wsize);

    close(fd);

    return 0;
}
