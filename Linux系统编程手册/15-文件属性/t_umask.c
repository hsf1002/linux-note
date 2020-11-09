#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>

#define FP_SPECIAL 1            /* Include set-user-ID, set-group-ID, and sticky bit information in returned string */
#define STR_SIZE sizeof("rwxrwxrwx")

#define MYFILE  "myfile"
#define MYDIR   "mydir"
#define FILE_PERMS  (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)  // rw-rw----
#define DIR_PERMS   (S_IRWXU | S_IRWXG | S_IRWXO)            // rwxrwxrwx
#define UMASK_SETTING (S_IWGRP | S_IXGRP | S_IWOTH | S_IXOTH) // ----wx-wx

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
 * umask

skydeiMac:15-文件属性 sky$ ./t_umask 
requested file perms:      rw-rw----
process umask:             ----wx-wx
actual file perms:         rw-r-----
requested dir perms:       rwxrwxrwx
process umask:             ----wx-wx
actual dir perms:          rwxr--r--
*/
int
main(int argc, const char **argv)
{
    int fd;
    struct stat sb;
    mode_t u;

    // 先设置umask
    umask(UMASK_SETTING);

    // 在创建文件和文件夹
    if (-1 == (fd = open(MYFILE, O_RDWR | O_CREAT | O_EXCL, FILE_PERMS)))
        perror("open error\n");
    if (-1 == mkdir(MYDIR, DIR_PERMS))
        perror("mkdir error\n");
    // 将umask保存，并清空原umask
    u = umask(0);

    if (-1 == stat(MYFILE, &sb))
        perror("stat file error");
    printf("requested file perms:      %s\n", file_perm_str(FILE_PERMS, 0));
    printf("process umask:             %s\n", file_perm_str(u, 0));
    printf("actual file perms:         %s\n", file_perm_str(sb.st_mode, 0));

    if (-1 == stat(MYDIR, &sb))
        perror("stat dir error");
    printf("requested dir perms:       %s\n", file_perm_str(DIR_PERMS, 0));
    printf("process umask:             %s\n", file_perm_str(u, 0));
    printf("actual dir perms:          %s\n", file_perm_str(sb.st_mode, 0));

    if (-1 == unlink(MYFILE))
        perror("unlink file error\n");
    
    if (-1 == rmdir(MYDIR))
        perror("rm dir error\n");
    
    exit(EXIT_SUCCESS);
}
