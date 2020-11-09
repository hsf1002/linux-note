#include "pty_fork.h"


#define MAX_SNAME 1000


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

