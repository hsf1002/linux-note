//#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <stddef.h>
#include <signal.h>
#include <ctype.h>

/**
 * 以F_SETLK方式加锁
 */
extern int
lock_region(int fd, int type, int whence, int start, off_t len);

/**
 * 以F_SETLKW方式加锁
 */
extern int
lock_region_wait(int fd, int type, int whence, int start, off_t len);
