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
#include <sys/time.h>
#include <sys/select.h>
#include <stddef.h>
#include <signal.h>
#include <ctype.h>
#include <termios.h>


/**
 * 设置为cbreak模式
 */
int
tty_set_cbreak(int fd, struct termios *prev_termios);

/**
 * 设置为原始模式
 */
int
tty_set_raw(int fd, struct termios *prev_termios);
