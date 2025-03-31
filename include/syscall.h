/*
 * Copyright (C) 2025 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* Sierra release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

#include "common.h"

struct sc_regs {
        uint32_t a1;
        uint32_t a2;
        uint32_t a3;
        uint32_t a4;
        uint32_t a5;
        uint32_t a6;

        /*
         * orig_ax is used on entry for:
         * - the syscall number (syscall, sysenter, int80)
         * - error_code stored by the CPU on traps and exceptions
         * - the interrupt number for device interrupts
         */
        uint32_t orig_ax;

        int32_t flags;
};


/* This is used purely for kernel trace the table of system call */
//typedef long (*sys_call_ptr_t)(const struct sc_regs *);
extern const char* sys_call_table[];
extern const int syscall_table_size;
int syscall(struct pcb_t*, uint32_t, struct sc_regs*);
int libsyscall(struct pcb_t*, uint32_t, uint32_t, uint32_t, uint32_t);
int __sys_ni_syscall(struct pcb_t*, struct sc_regs*);

