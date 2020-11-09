//#define _GNU_SOURCE
#define _XOPEN_SOURCE 600
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
#include <sys/time.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <stddef.h>
#include <signal.h>
#include <ctype.h>
#include <termios.h>
#include <errno.h>
#include "pty_master_open.h"

#define MAX_SNAME 1000


/**
 *  实现pty_fork
 */
pid_t
pty_fork(int *master_fd, char *slave_name, size_t sn_len, const struct termios *slave_termios, const struct winsize *slave_ws);
