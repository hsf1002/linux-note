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
#include "tty_func.h"


#define MAX_SNAME 1000
#define BUF_SIZE 256

struct termios tty_orig;


/**
 *  实现pty_fork
 */
pid_t
pty_fork(int *master_fd, char *slave_name, size_t sn_len, const struct termios *slave_termios, const struct winsize *slave_ws)
{
    int m_fd; // master
    int s_fd; // slave
    int saved_errno;
    pid_t child_pid;
    char sl_name[MAX_SNAME];

    // 1. 打开伪终端主设备，获取从设备名称
    if (-1 == (m_fd = pty_master_open(sl_name, MAX_SNAME)))
        return -1;
    // 2. 返回从设备名称给调用者
    if (NULL != slave_name)
    {
        if (strlen(sl_name) < sn_len)
        {
            strncpy(slave_name, sl_name, sn_len);
        }
        else
        {
            close(m_fd);
            errno = saved_errno;
            return -1;
        }
    }

    // 3. 创建子进程
    child_pid = fork();

    if (-1 == child_pid)
    {
        saved_errno = errno;
        close(m_fd);
        errno = saved_errno;
        return -1;
    }

    // 4. 父进程，返回主设备文件描述符给调用者
    if (0 != child_pid)
    {
        *master_fd = m_fd;
        return child_pid;
    }

    // 5. 子进程继续进行如下步骤
    // 5.1 创建新会话，子进程成为新会话的首进程，且失去控制终端（如果有的话）
    if (-1 == setsid())
        exit(EXIT_FAILURE);
    // 5.2 关闭主设备文件描述符，子进程不再需要
    close(m_fd);

    // 5.3 打开伪终端从设备，由于子进程失去了控制终端，这一部将导致伪终端从设备成为子进程的控制终端
    if (-1 == (s_fd = open(sl_name, O_RDWR)))
        exit(EXIT_FAILURE);
    
#ifdef TIOCSCTTY // BSD平台需要
    if (-1 == ioctl(s_fd, TIOCSCTTY, 0))
        exit(EXIT_FAILURE);
#endif 
    // 5.4 设置从设备的终端属性
    if (NULL != slave_termios)
        if (-1 == tcsetattr(s_fd, TCSANOW, slave_termios))
            exit(EXIT_FAILURE);
    // 5.5 设置从设备的终端窗口大小
    if (NULL != slave_ws)
        if (-1 == ioctl(s_fd, TIOCSWINSZ, slave_ws))
            exit(EXIT_FAILURE);
    // 5.6 复制从设备的文件描述符成为子进程的标准输入、标准输出、错误输出
    if (dup2(s_fd, STDIN_FILENO) != STDIN_FILENO)
        exit(EXIT_FAILURE);
    
    if (dup2(s_fd, STDOUT_FILENO) != STDOUT_FILENO)
        exit(EXIT_FAILURE);

    if (dup2(s_fd, STDERR_FILENO) != STDERR_FILENO)
        exit(EXIT_FAILURE);
}

/**
 *  进程退出时重置终端模式
 */ 
static void
tty_reset(void)
{
    if (-1 == tcsetattr(STDIN_FILENO, TCSANOW, &tty_orig))
        perror("tcsetattr err");
}

/**
 * 
    script(1)程序的简单实现


cc -g -Wall -o pty_script pty_script.c libptymasteropen.so libtty.so

// 当前伪终端名称
skydeiMac:64-伪终端 sky$ tty 
/dev/ttys003
// 当前登录shell的进程ID
skydeiMac:64-伪终端 sky$ echo $$
76173
// 启动script程序，该程序会启动一个shell进程
skydeiMac:64-伪终端 sky$ ./pty_script
bash-3.2$ tty
/dev/ttys002
bash-3.2$ echo $$
35081

// 显示有关两个shell以及script进程间的相关信息，最后关闭由script程序启动的shell
bash-3.2$ ps -p 76173 -p 35081 -C pty_script -o "pid ppid sid tty cmd"
...
exit

cat typescript
...

 */
int main(int argc, char *argv[])
{
    char slave_name[MAX_SNAME];
    char *shell;
    int master_fd;
    int script_fd;
    struct winsize ws;
    fd_set in_fds;
    char buf[BUF_SIZE];
    ssize_t num_read;
    pid_t child_pid;

    // 1. 获取并保存标准输入当前的终端属性和窗口大小
    if (-1 == tcgetattr(STDIN_FILENO, &tty_orig))
        perror("tcgetattr error");
    
    if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) < 0)
        perror("ioctl TIOCGWINSZ error");

    // 2. 创建伪终端的子进程
    child_pid = pty_fork(&master_fd, slave_name, MAX_SNAME, &tty_orig, &ws);
    
    switch (child_pid)
    {
        // 创建进程失败
        case -1:
            perror("pty_fork error");
            break;
        // 3. 子进程：在伪终端的从设备上执行shell
        case 0:
        {
            shell = getenv("SHELL");
            if (NULL == shell || *shell == '\0')
                shell = "/bin/sh";
            execlp(shell, shell, (char *)NULL);
            perror("execlp error");
        }
        break;
        // 4. 父进程：在用户终端和伪终端主设备之间转发数据
        default:
        {
            // 4.1 打开/创建script输出文件
            if (-1 == (script_fd = open((argc > 1) ? argv[1] : "typescript", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)))
                perror("open typescript err");
            
            // 4.2 将终端设置为原始模式
            tty_set_raw(STDIN_FILENO, &tty_orig);

            // 4.3 注册进程退出函数
            if (0 != atexit(tty_reset))
                perror("atexit err");
            
            for (;;)
            {
                FD_ZERO(&in_fds);
                FD_SET(STDIN_FILENO, &in_fds);
                FD_SET(master_fd, &in_fds);

                // 监视终端标准输入和伪终端主设备上的输入
                if (-1 == select(master_fd + 1, &in_fds, NULL, NULL, NULL))
                    perror("select err");
                
                // 4.4 若标准输入有数据，先读取再写入到伪终端
                if (FD_ISSET(STDIN_FILENO, &in_fds))
                {
                    if ((num_read = read(STDIN_FILENO, buf, BUF_SIZE)) <= 0)
                        exit(EXIT_SUCCESS);
                    
                    if (write(master_fd, buf, num_read) != num_read)
                        perror("write master_fd err");
                }

                // 4.5 若伪终端主设备有数据，先读取再写入到终端输出和输出文件
                if (FD_ISSET(master_fd, &in_fds))
                {
                    if ((num_read = read(master_fd, buf, BUF_SIZE)) <= 0)
                        exit(EXIT_SUCCESS);
                    
                    if (write(STDIN_FILENO, buf, num_read) != num_read)
                        perror("write stdin err");
                    if (write(script_fd, buf, num_read) != num_read)
                        perror("write script_fd err");
                }
            }
        }
        break;
    }
    
    exit(EXIT_SUCCESS);
}

