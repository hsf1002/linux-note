#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/uio.h>

/**
 * 
 * 分散输入的例子
 */
int
main(int argc, char *argv[])
{
    int fd;
    struct iovec iov[3];
    //struct stat first_buf;
    char first_buf;
    int second_buf;
#define STR_SIZE 100
    char third_buf[STR_SIZE];
    ssize_t num_read;
    ssize_t required;

    if (argc != 2 || strcmp(argv[1], "--help") == 0)
        fprintf(stderr, "usage: %s file \n", argv[0]);

    if (-1 == (fd = open(argv[1], O_RDONLY)))
        perror("open error");

    required = 0;

    iov[0].iov_base = &first_buf;
    iov[0].iov_len = sizeof(char);//sizeof(struct stat);
    required += iov[0].iov_len;
    printf("0----- %d\n", required);

    iov[1].iov_base = &second_buf;
    iov[1].iov_len = sizeof(second_buf);
    required += iov[1].iov_len;
    printf("1----- %d\n", required);

    iov[2].iov_base = &third_buf;
    iov[2].iov_len = STR_SIZE;
    required += iov[2].iov_len;
    printf("2----- %d\n", required);

    if (-1 == (num_read = readv(fd, iov, 3)))
        perror("readv error");
    if (num_read < required)
        printf("read fewer than required");
   
    printf("third_buf_len = %d \n", strlen(third_buf));
    ((char *)iov[1].iov_base)[iov[1].iov_len] = '\0';
    ((char *)iov[2].iov_base)[strlen(third_buf)] = '\0';
    
    printf("total bytes requsted: %ld, bytes read: %ld \n", (long)required, (long)num_read);

    printf("iov[0].base = %c\n", *(char* )iov[0].iov_base);
    printf("iov[1].base = %s\n", (char *)iov[1].iov_base);
    printf("iov[1].base = %d\n", atoi((char *)iov[1].iov_base));
    printf("iov[2].base = %s\n", (char *)iov[2].iov_base);

    exit(EXIT_SUCCESS);
}

