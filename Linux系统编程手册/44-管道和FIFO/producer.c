
#include "share.h"

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
  memset(shared, 0, sizeof(struct shm_data));
  semaphore_init(semid);

  while(1)
  {
    // 获取一个资源
    semaphore_p(semid);

    // 如果有数据，释放一个资源，睡眠1s
    if(shared->datalength > 0)
    {
      semaphore_v(semid);
      sleep(1);
    } 
    // 如果没有数据，添加
    else 
    {
      printf("how many integers to caculate : ");
      scanf("%d", &shared->datalength);
      // 要输入几个数字
      if(shared->datalength > MAX_NUM)
      {
        perror("too many integers.");
        shared->datalength = 0;
        semaphore_v(semid);
        exit(1);
      }
      
      // 依次输入每个数据
      for(i=0;i<shared->datalength;i++)
      {
        printf("Input the %d integer : ", i);
        scanf("%d",&shared->data[i]);
      }
      semaphore_v(semid);
    }
  }
}
