#include "pty_master_open_bsd.h"




/**
 * 
 * 打开伪终端主设备的实现(BSD风格)
 */
int pty_master_open_bsd(char *slave_name, size_t sn_len)
{
  int m_fd;
  int n;
  char *x;
  char *y;
  char master_name[PTY_NAME_LEN];

  // 指定的长度太小，放不下
  if (PTY_NAME_LEN > sn_len)
  {
    errno = EOVERFLOW;
    return -1;
  }

  memset(master_name, 0x00, PTY_NAME_LEN);
  strncpy(master_name, PTYM_PREFIX, PTY_PREFIX_LEN);

  for (x=X_RANGE; *x!='\0'; ++x)
  {
    master_name[PTY_PREFIX_LEN] = *x;

    for (y=Y_RANGE; *y!='\0'; ++y)
    {
      // 组装主设备名称
      master_name[PTY_PREFIX_LEN + 1] = *y;

      // 依次打开主设备
      if (-1 == (m_fd = open(master_name, O_RDWR)))
      {
        // 通常表示已经遍历整个主设备的组合，未找到空闲设备
        if (ENOENT == errno)
          return -1;
        // 忽略直接打开下一个设备即可
        else 
          continue;
      }
      else
      {
        // 组装从设备名称
        n = snprintf(slave_name, sn_len, "%s%c%c", PTYS_PREFIX, *x, *y);

        if (n >= sn_len)
        {
          errno = EOVERFLOW;
          return -1;
        }
        else if (n == -1)
        {
          return -1;
        }

        return m_fd;
      }
    }
  }
  return -1;
}
