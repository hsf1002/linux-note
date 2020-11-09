#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/stat.h>
#include "get_num.h"


/*

skydeiMac:04-通用的IO模型 sky$ touch tfile
skydeiMac:04-通用的IO模型 sky$ ./seek_io tfile s100000 wabc
s100000: seek success
wabc: wrote 3 bytes
skydeiMac:04-通用的IO模型 sky$ ll -h tfile
-rw-r--r--  1 sky  staff    98K 10 26 21:43 tfile
skydeiMac:04-通用的IO模型 sky$ ./seek_io tfile s10000 R5
s10000: seek success
R5:
00 00 00 00 00
*/
int
main(int argc, char *argv[])
{
    size_t len;
    off_t offset;
    int fd;
    int ap;
    int j;
    char *buf;
    ssize_t num_read;
    ssize_t num_write;

    if (argc < 3 || strcmp(argv[1], "--help") == 0)
        fprintf(stderr, "%s file {r<length>|R<length>|w<string>|s<offset>}...", argv[0]);
    if (-1 == (fd = open(argv[1], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)))
        perror("open error");
    // 循环解析第2以后的参数，进行读，写，移操作
    for (ap=2; ap<argc; ap++)
    {
        switch (argv[ap][0])
        {
            case 'r':
            case 'R':
            {
                len = getLong(&argv[ap][1], GN_ANY_BASE, argv[ap]);
                if (NULL == (buf = (char *)malloc(len)))
                    perror("malloc error");
                
                if (-1 == (num_read = read(fd, buf, len)))
                    perror("read error");
                if (num_read == 0)
                    printf("%s: end-of-file\n", argv[ap]);
                else
                {
                    printf("%s: \n", argv[ap]);

                    for (j=0; j<num_read; j++)
                    {
                        if ('r' == argv[ap][0])
                            printf("%c", isprint((unsigned char)buf[j]) ? buf[j] : '?');
                        // R以16进制显示
                        else
                            printf("%02x ", (unsigned int)buf[j]);
                    }
                    printf("\n");
                }
                
                free(buf);
            }
            break;
            case 'w':
            case 'W':
            {
                if (-1 == (num_write = write(fd, &argv[ap][1], strlen(&argv[ap][1]))))
                    perror("write error");
                printf("%s: wrote %ld bytes\n", argv[ap], (long)num_write);
            }
            break;
            case 's':
            case 'S':
            {
                offset = getLong(&argv[ap][1], GN_ANY_BASE, argv[ap]);
                if (-1 == (lseek(fd, offset, SEEK_SET)))
                    perror("lseek error");
                printf("%s: seek success\n", argv[ap]);
            }
            break;
        default:
            printf("argument must start with [rRwWsS]\n");
            break;
        }
    }

    exit(EXIT_SUCCESS);
}
