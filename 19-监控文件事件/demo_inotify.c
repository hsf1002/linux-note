#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/inotify.h>

#define BUF_LEN  (10 * sizeof(struct inotify_event) + NAME_MAX +1)

/**
 * 
 * 将事件列表显示出来
 */
static void
display_inotify_events(struct inotify_event *i)
{
    printf("    wd = %2d; ", i->wd);

    if (i->cookie > 0)
        printf("cookie = %4d; ", i->cookie);
    
    printf("mask = ");

    if (i->mask & IN_ACCESS)
        printf("IN_ACCESS");
    if (i->mask & IN_ATTRIB)
        printf("IN_ATTRIB");
    if (i->mask & IN_CLOSE_NOWRITE)
        printf("IN_CLOSE_NOWRITE");
    if (i->mask & IN_CLOSE_WRITE)
        printf("IN_CLOSE_WRITE");
    if (i->mask & IN_CREATE)
        printf("IN_CREATE");
    if (i->mask & IN_DELETE)
        printf("IN_DELETE");
    if (i->mask & IN_DELETE_SELF)
        printf("IN_DELETE_SELF");
    if (i->mask & IN_IGNORED)
        printf("IN_IGNORED");
    if (i->mask & IN_ISDIR)
        printf("IN_ISDIR");
    if (i->mask & IN_MODIFY)
        printf("IN_MODIFY");
    if (i->mask & IN_MOVE_SELF)
        printf("IN_MOVE_SELF");
    if (i->mask & IN_MOVE_FROM)
        printf("IN_MOVE_FROM");
    if (i->mask & IN_MOVE_TO)
        printf("IN_MOVE_TO");
    if (i->mask & IN_OPEN)
        printf("IN_OPEN");
    if (i->mask & IN_Q_OVERFLOW)
        printf("IN_Q_OVERFLOW");
    if (i->mask & IN_UNMOUNT)
        printf("IN_UNMOUNT");

    printf("\n");

    // 打印出所监控的文件名
    if (i->len > 0)
        printf("     name = %s\n", i->name);
}

/**
 *   
 *  展示对inotify的运用

echo "world" >> hi
ll hi
-rw-rw-r-- 1 hefeng hefeng 13 12月 24 10:56 hi
chmod +x hi
rm hi

./a.out hi
Watching hi using wd 1
Read 16 bytes from inotify fd
    wd = 1; mask = IN_OPEN 
Read 16 bytes from inotify fd
    wd = 1; mask = IN_MODIFY 
Read 16 bytes from inotify fd
    wd = 1; mask = IN_CLOSE_WRITE 
Read 16 bytes from inotify fd
    wd = 1; mask = IN_ATTRIB 
Read 48 bytes from inotify fd
    wd = 1; mask = IN_ATTRIB 
    wd = 1; mask = IN_DELETE_SELF 
    wd = 1; mask = IN_IGNORED 
^C


*/
int
main(int argc, char *argv[])    
{
    int inotify_fd;
    int wd;
    char buf[BUF_LEN];
    ssize_t num_read;
    char *p;
    struct inotify_event *event;

    if (argc < 2 || 0 == strcmp(argv[1], "--help"))
        fprintf(stderr, "%s pathname...\n", argv[0]);
    
    // 创建inotify实例
    if (-1 == (inotify_fd = inotify_init()))
        perror("inotify_init error");

    // 给inotify实例添加监控项
    for (int i=1; i<argc; ++i)
    {
        // 侦测所有事件
        if (-1 == (wd = inotify_add_watch(inotify_fd, argv[i], IN_ALL_EVENTS)))
            perror("inotify_add_watch error");
        printf("watching %s using wd=%d\n", argv[i], wd);
    }

    // 无限循环，一旦有事件读取出来，立即打印
    for (;;)
    {
        if (0 == (num_read = read(inotify_fd, buf, BUF_LEN)))
            perror("read from inotify fd returned 0");
        else if (-1 == num_read)
            perror("read error");
        
        printf("read %ld bytes from inotify fd\n", (long)num_read);

        for (p=buf; p<buf+num_read)
        {
            event = (struct inotify_event *)p;
            display_inotify_events(event);
            p += sizeof(struct inotify_event) + event->len;
        }
    }

    exit(EXIT_SUCCESS);
}

