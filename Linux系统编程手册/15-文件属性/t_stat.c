#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>

#define FP_SPECIAL 1            /* Include set-user-ID, set-group-ID, and sticky bit information in returned string */
#define STR_SIZE sizeof("rwxrwxrwx")

/* 
    将数值转换为可读字符串：rwxrwxrwx的形式
*/
char *          
file_perm_str(mode_t perm, int flags)
{
    static char str[STR_SIZE];

    /* If FP_SPECIAL was specified, we emulate the trickery of ls(1) in
       returning set-user-ID, set-group-ID, and sticky bit information in
       the user/group/other execute fields. This is made more complex by
       the fact that the case of the character displayed for this bits
       depends on whether the corresponding execute bit is on or off. */

    snprintf(str, STR_SIZE, "%c%c%c%c%c%c%c%c%c",
        (perm & S_IRUSR) ? 'r' : '-', (perm & S_IWUSR) ? 'w' : '-',
        (perm & S_IXUSR) ?
            (((perm & S_ISUID) && (flags & FP_SPECIAL)) ? 's' : 'x') :
            (((perm & S_ISUID) && (flags & FP_SPECIAL)) ? 'S' : '-'),
        (perm & S_IRGRP) ? 'r' : '-', (perm & S_IWGRP) ? 'w' : '-',
        (perm & S_IXGRP) ?
            (((perm & S_ISGID) && (flags & FP_SPECIAL)) ? 's' : 'x') :
            (((perm & S_ISGID) && (flags & FP_SPECIAL)) ? 'S' : '-'),
        (perm & S_IROTH) ? 'r' : '-', (perm & S_IWOTH) ? 'w' : '-',
        (perm & S_IXOTH) ?
            (((perm & S_ISVTX) && (flags & FP_SPECIAL)) ? 't' : 'x') :
            (((perm & S_ISVTX) && (flags & FP_SPECIAL)) ? 'T' : '-'));

    return str;
}


/**
 * 
 */
static void
display_stat_info(const struct stat *sb)
{
    printf("file type:                ");

    switch (sb->st_mode & S_IFMT)
    {
        case S_IFREG:
        {
            printf("regular file\n");
        }
        break;
        case S_IFDIR:
        {
            printf("directory file\n");
        }
        break;
        case S_IFCHR:
        {
            printf("character device\n");
        }
        break;
        case S_IFBLK:
        {
            printf("block device\n");
        }
        break;
        case S_IFLNK:
        {
            printf("symbolic soft link file\n");
        }
        break;
        case S_IFIFO:
        {
            printf("fifo or pipe\n");
        }
        break;
        case S_IFSOCK:
        {
            printf("socket\n");
        }
        break;
        default:
        {
            printf("unknown type\n");
        }
        break;
    }

    printf("device containing i-node: major=%ld, minor=%ld\n", (long)major(sb->st_dev), (long)minor(sb->st_dev));
    printf("i-node number:            %ld\n", (long)sb->st_ino);
    printf("mode:                     %lo (%s)\n", (unsigned long)sb->st_mode, file_perm_str(sb->st_mode, 0));
    
    if (sb->st_mode & (S_ISUID | S_ISGID | S_ISVTX))
        printf("special bits set:     %s%s%s\n", \
        (sb->st_mode & S_ISUID) ? "set-UID" : "", \
        (sb->st_mode & S_ISGID) ? "set-GID" : "",\
        (sb->st_mode & S_ISVTX) ? "sticky" : "");

    printf("number of (hard) links:    %ld\n", (long)sb->st_nlink);
    printf("ownership:                 UID=%ld, GID=%ld\n", (long)sb->st_uid, (long)sb->st_gid);

    if (S_ISCHR(sb->st_mode) || S_ISBLK(sb->st_mode))
        printf("device number (st_rdev:) major=%ld, minor=%ld\n", (long)major(sb->st_rdev), (long)minor(sb->st_rdev));
    
    printf("file size:                 %lld bytes\n", (long long)sb->st_size);
    printf("optical IO block size:     %ld bytes\n", (long)sb->st_blksize);
    printf("512B blocks allocated:     %lld\n", (long long)sb->st_blocks);
    printf("last file access:          %s", ctime(&sb->st_atime));
    printf("last file modified:        %s", ctime(&sb->st_mtime));
    printf("last status changed:       %s", ctime(&sb->st_ctime));
}

/**
 *   
 *  获取文件stat信息

./a.out README.md 
file type:                regular file
device containing i-node: major=1, minor=8
i-node number:            7274847
mode:                     100644 (rw-r--r--)
number of (hard) links:    1
ownership:                 UID=501, GID=20
file size:                 12896 bytes
optical IO block size:     4096 bytes
512B blocks allocated:     32
last file access:          Sat Nov 23 21:04:12 2019
last file modified:        Sat Nov 23 21:04:12 2019
last status changed:       Sat Nov 23 21:04:12 2019

*/
int
main(int argc, char *argv[])    
{
    struct stat sb;
    bool stat_link;
    int fname;

    // 如果是连接文件，第二个参数是-l，第三个参数是文件名
    // 如果是普通文件，第二个参数是文件名
    stat_link = (argc > 1) && 0 == strcmp(argv[1], "-l");

    fname = stat_link ? 2 : 1;

    if (fname > argc || (argc > 1 && 0 == strcmp(argv[1], "--help")))
        fprintf(stderr, "%s [l] file \n,     -l = use lstat() instead of stat() \n", argv[0]);
    
    if (stat_link)
    {
        if (-1 == lstat(argv[fname], &sb))
            perror("lstat error");
    }
    else 
    {
        if (-1 == stat(argv[fname], &sb))
            perror("stat error");
    }

    display_stat_info(&sb);

    exit(EXIT_SUCCESS);
}

