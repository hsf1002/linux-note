
#ifndef BINARY_SEMS_H           /* Prevent accidental double inclusion */
#define BINARY_SEMS_H

#include "sem_un.h"

/* Use SEM_UNDO during semop()? */
extern bool bs_use_undo;    
/* Retry if semop() interrupted by signal handler? */        
extern bool bs_retry;          

int init_sem_available(int sem_id, int sem_num);

int init_sem_inuse(int sem_id, int sem_num);

int reserve_sem(int sem_id, int sem_num);

int release_sem(int sem_id, int sem_num);

#endif
