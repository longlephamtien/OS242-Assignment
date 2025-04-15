#include <common.h>
#include <syscall.h>
#include <stdlib.h>

int __sys_xxxhandler(struct pcb_t *caller, struct sc_regs* reg)
{
    
    printf("The xxxhandler system call parameter %d\n", reg->a1);
    return 0;
};
