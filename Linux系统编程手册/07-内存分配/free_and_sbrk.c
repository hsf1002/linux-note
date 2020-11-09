#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "get_num.h"


#define MAX_ALLOC  1000000


/**
 *   调用free对program break的影响

skydeiMac:07-内存分配 sky$ ./free_and_sbrk 1000 10240 2

initial program break:           0x10cdbc000
allocating 1000*10240 bytes
program break is now:           0x10cdbc000
after free(), program break is:   0x10cdbc000
skydeiMac:07-内存分配 sky$ ./free_and_sbrk 1000 10240 1 1 999

initial program break:           0x109cd8000
allocating 1000*10240 bytes
program break is now:           0x109cd8000
after free(), program break is:   0x109cd8000
skydeiMac:07-内存分配 sky$ ./free_and_sbrk 1000 10240 1 500 1000

initial program break:           0x10701d000
allocating 1000*10240 bytes
program break is now:           0x10701d000
after free(), program break is:   0x10701d000

*/
int
main(int argc, char *argv[])    
{
    char *ptr[MAX_ALLOC];
    int free_step;
    int free_min;
    int free_max;
    int block_size;
    int num_alloc;
    int i;

    printf("\n");

    if (argc < 3 || 0 == strcmp(argv[1], "--help"))
        fprintf(stderr, "num_alloc block_size [step [min [max] ] ]\n", argv[0]);
    
    if ((num_alloc = getInt(argv[1], GN_GT_0, "num_alloc")) > MAX_ALLOC)
        fprintf(stderr, "num_alloc > %d\n", MAX_ALLOC);
    
    block_size = getInt(argv[2], GN_GT_0 | GN_ANY_BASE, "block_size");

    free_step = (argc > 3) ? getInt(argv[3], GN_GT_0, "step") : 1;
    free_min = (argc > 4) ? getInt(argv[4], GN_GT_0, "min") : 1;
    free_max = (argc > 5) ? getInt(argv[5], GN_GT_0, "max") : num_alloc;

    if (free_max > num_alloc)
        perror("free_max > num_alloc\n");
    printf("initial program break:           %10p\n", sbrk(0));

    printf("allocating %d*%d bytes\n", num_alloc, block_size);
    for (i=0; i<num_alloc; ++i)
    {
        if (NULL == (ptr[i] = (char *)malloc(block_size)))
            perror("malloc error");
    }

    printf("program break is now:           %10p\n", sbrk(0));
    for (i=free_min-1; i<free_max; i+=free_step)
        free(ptr[i]);
    
    printf("after free(), program break is:   %10p\n", sbrk(0));

    exit(EXIT_SUCCESS);
}

