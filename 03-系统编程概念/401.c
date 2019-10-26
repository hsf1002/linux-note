#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>


#ifndef BUF_SIZE
#define BUF_SIZE 1024
#endif

/**
 * 
 *  ./copy oldfile newfile
 */
int 
main(int argc, char *argv[])
{
    int fd_in;
    int fd_out;
    int flags;
    mode_t perms;
    ssize_t num_read;
    char buf[BUF_SIZE];

    if (argc != 3 || strcmp(argv[1], "--help") == 0)
        fprintf(stderr, "%s oldfile newfile \n", argv[0]);
    
    if (-1 == (fd_in = open(argv[1], O_RDONLY)))
        perror("open oldfile error");
    
    flags = O_CREAT | O_WRONLY | O_TRUNC;
    perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH; // rw-rw-rw-

    if (-1 == (fd_out = open(argv[2], flags, perms)))
        perror("open newfile error");
    
    while ((num_read = read(fd_in, buf, BUF_SIZE)) > 0)
        if (write(fd_out, buf, num_read) != num_read)
            perror("write error");

    if (-1 == num_read)
        perror("read error");
    if (-1 == close(fd_in))
        perror("close oldfile error");
    if (-1 == close(fd_out))
        perror("close newfile error");
    
    exit(EXIT_SUCCESS);
}

