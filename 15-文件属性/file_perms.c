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

