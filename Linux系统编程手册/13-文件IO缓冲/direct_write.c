#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <malloc.h>

#define BUF_SIZE 4096

#define BUF_CONTENT_BEFORE_READ "In line with the promise of this age, I want to exhort each of your gradutes here"
#define BUF_CONTENT_AFTER_WRITE "to take on an issue, a complex issue, a deep inequality"


/**
 * cc direct_write.c -o direct_write
 *  read/write的大小必须是 BUF_SIZE
 * 
./direct_write 
read bytes_read = 4096, size = 81, buffer = In line with the promise of this age, I want to exhort each of your gradutes here 
read2 bytes_read = 4096, size = 55, buffer = to take on an issue, a complex issue, a deep inequality 
*/
int main() 
{
    int fd;
    char *buffer;
    ssize_t bytes_read, bytes_written;

    // 分配 4096 字节内存，以512整数对齐，直接IO 的数据必须先对齐
    if (posix_memalign((void **)&buffer, 512, BUF_SIZE) != 0) 
    {
        perror("posix_memalign error");
        return 1;
    }

    // 以直接IO方式 O_DIRECT 打开文件
    fd = open("testfile.txt", O_RDWR | O_CREAT | O_DIRECT, 0666);
    if (fd == -1) 
    {
        perror("open error");
        return 1;
    }

    // 覆盖写数据
    memset(buffer, 0x00, BUF_SIZE);
    strcpy(buffer, BUF_CONTENT_BEFORE_READ);
    bytes_written = write(fd, buffer, BUF_SIZE);
    if (bytes_written == -1) {
        perror("write error");
        return 1;
    }

    // 读数据
    lseek(fd, 0, SEEK_SET);
    bytes_read = read(fd, buffer, BUF_SIZE);
    if (bytes_read == -1) {
        perror("read error");
        return 1;
    }
    else
    {
        printf("read bytes_read = %d, size = %d, buffer = %s \n", bytes_read, strlen(buffer), buffer);
    }

    // 覆盖写数据
    memset(buffer, 0x00, BUF_SIZE);
    strcpy(buffer, BUF_CONTENT_AFTER_WRITE);

    lseek(fd, 0, SEEK_SET);
    bytes_written = write(fd, buffer, BUF_SIZE);
    if (bytes_written == -1) {
        perror("write2 error");
        return 1;
    }

    // 读数据
    lseek(fd, 0, SEEK_SET);
    bytes_read = read(fd, buffer, BUF_SIZE);
    if (bytes_read == -1) {
        perror("read2 error");
        return 1;
    }
    else
    {
        printf("read2 bytes_read = %d, size = %d, buffer = %s \n", bytes_read, strlen(buffer), buffer);
    }    

    close(fd);
    free(buffer);

    return 0;
}
