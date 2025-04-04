#include <common.h>
#include <syscall.h>
#include <stdlib.h>

int __sys_xxx_handler(struct pcb_t *caller, struct sc_regs* reg)
{
    /* data */
    printf("The first system call parameter %d\n", reg->a1);
    return 0;
};

