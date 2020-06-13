
#include "share.h"



/*
./producer 
how many integers to caculate : 3
Input the 0 integer : 1
Input the 1 integer : 2
Input the 2 integer : 3




444
^C
skydeiMac:44-管道和FIFO sky$ ./consumer 
1+2+3=6
no tasks, waiting.
no tasks, waiting.
*/
int main() 
{
  void *shm = NULL;
  struct shm_data *shared = NULL;
  int shmid = get_shmid();
  int semid = get_semaphoreid();
  int i;
  
  // 创建共享内存
  shm = shmat(shmid, (void*)0, 0);
  if(shm == (void*)-1)
  {
    exit(0);
  }
  shared = (struct shm_data*)shm;

  while(1)
  {
    // 获取一个资源
    semaphore_p(semid);

    // 如果有数据，取出来
    if(shared->datalength > 0)
    {
      int sum = 0;
      // 对所有数据求和
      for(i=0;i<shared->datalength-1;i++)
      {
        printf("%d+",shared->data[i]);
        sum += shared->data[i];
      }
      printf("%d",shared->data[shared->datalength-1]);
      sum += shared->data[shared->datalength-1];
      printf("=%d\n",sum);
      // 清空共享内存的数据
      memset(shared, 0, sizeof(struct shm_data));
      // 释放一个资源
      semaphore_v(semid);
    } 
    // 如果没有数据，等待1s
    else 
    {
      semaphore_v(semid);
      printf("no tasks, waiting.\n");
      sleep(1);
    }
  }
}
