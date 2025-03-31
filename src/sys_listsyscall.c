/*
 * Copyright (C) 2025 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* Sierra release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

#include "syscall.h"

int __sys_listsyscall(struct pcb_t *caller, struct sc_regs* reg)
{
   for (int i = 0; i < syscall_table_size; i++)
       printf("%s\n",sys_call_table[i]); 

   return 0;
}
