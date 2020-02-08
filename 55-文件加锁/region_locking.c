#include "region_locking.h"

/**
 * 加记录锁
 */
static int
lock_reg(int fd, int cmd, int type, int whence, int start, off_t len)
{
    struct flock fl;
    
    fl.l_type = type;
    fl.l_whence = whence;
    fl.l_start = start;
    fl.l_len = len;

    return fcntl(fd, cmd, &fl);
}

/**
 * 以F_SETLK方式加锁
 */
extern int
lock_region(int fd, int type, int whence, int start, off_t len)
{
    return lock_reg(fd, F_SETLK, type, whence, start, len);
}

/**
 * 以F_SETLKW方式加锁
 */
extern int
lock_region_wait(int fd, int type, int whence, int start, off_t len)
{
    return lock_reg(fd, F_SETLKW, type, whence, start, len);
}

/**
 * 如果已经加锁，返回持有锁的进程PID，没有加锁，返回0
 * 
 */
extern int
lock_is_locked(int fd, int type, int whence, int start, off_t len)
{
    struct flock fl;
    
    fl.l_type = type;
    fl.l_whence = whence;
    fl.l_start = start;
    fl.l_len = len;

    if (-1 == fcntl(fd, F_GETLK, &fl))
        return -1;

    return (fl.l_type == F_UNLCK) ? 0 : fl.l_pid;
}
