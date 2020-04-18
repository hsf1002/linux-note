#include "pty_master_open.h"


/**
 * 
 * 打开伪终端主设备的实现
 */
int pty_master_open(char *slave_name, size_t sn_len)
{
    int m_fd, saved_errno;
    char *p;

    // 1. 打开伪终端未使用的主设备
    if (-1 == (m_fd = posix_openpt(O_RDWR | O_NOCTTY)))
        return -1;
    
    // 2. 修改伪终端主设备关联的从设备的属主和权限
    if (-1 == grantpt(m_fd))
    {
        saved_errno = errno;
        close(m_fd);
        errno = saved_errno;
        return -1;
    }

    // 3. 解锁伪终端主设备关联的从设备
    if (-1 == unlockpt(m_fd))
    {
        saved_errno = errno;
        close(m_fd);
        errno = saved_errno;
        return -1;
    }

    // 4. 获取伪终端从设备名称
    if (NULL == (p = ptsname(m_fd)))
    {
        saved_errno = errno;
        close(m_fd);
        errno = saved_errno;
        return -1;
    }

    if (strlen(p) < sn_len)
    {
        strncpy(slave_name, p, sn_len);
    }
    else
    {
        close(m_fd);
        errno = EOVERFLOW;
        return -1;
    }

    return m_fd;
}
