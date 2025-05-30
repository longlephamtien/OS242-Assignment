/*
 * Copyright (C) 2025 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* Sierra release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

#include "string.h"
#include "mm.h"
#include "syscall.h"
#include "libmem.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

static pthread_mutex_t mmvm_lock = PTHREAD_MUTEX_INITIALIZER;

/*enlist_vm_freerg_list - add new rg to freerg_list
 *@mm: memory region
 *@rg_elmt: new region
 *
 */
int enlist_vm_freerg_list(struct mm_struct *mm, struct vm_rg_struct *rg_elmt)
{
  if (rg_elmt->rg_start >= rg_elmt->rg_end)
    return -1;

  struct vm_rg_struct **prev = &mm->mmap->vm_freerg_list;
  struct vm_rg_struct *cur = mm->mmap->vm_freerg_list;

  // Insert in sorted order
  while (cur && cur->rg_start < rg_elmt->rg_start)
  {
    prev = &cur->rg_next;
    cur = cur->rg_next;
  }
  rg_elmt->rg_next = cur;
  *prev = rg_elmt;

  // Merge with next if adjacent
  if (rg_elmt->rg_next && rg_elmt->rg_end == rg_elmt->rg_next->rg_start)
  {
    struct vm_rg_struct *to_merge = rg_elmt->rg_next;
    rg_elmt->rg_end = to_merge->rg_end;
    rg_elmt->rg_next = to_merge->rg_next;
    free(to_merge);
  }
  // Merge with previous if adjacent
  if (prev != &mm->mmap->vm_freerg_list)
  {
    struct vm_rg_struct *prev_rg = mm->mmap->vm_freerg_list;
    while (prev_rg && prev_rg->rg_next != rg_elmt)
      prev_rg = prev_rg->rg_next;
    if (prev_rg && prev_rg->rg_end == rg_elmt->rg_start)
    {
      prev_rg->rg_end = rg_elmt->rg_end;
      prev_rg->rg_next = rg_elmt->rg_next;
      free(rg_elmt);
    }
  }
  return 0;
}

/*get_symrg_byid - get mem region by region ID
 *@mm: memory region
 *@rgid: region ID act as symbol index of variable
 *
 */
struct vm_rg_struct *get_symrg_byid(struct mm_struct *mm, int rgid)
{
  if (rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
    return NULL;

  return &mm->symrgtbl[rgid];
}

/*__alloc - allocate a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *@alloc_addr: address of allocated memory region
 *
 */
int __alloc(struct pcb_t *caller, int vmaid, int rgid, int size, int *alloc_addr)
{
  pthread_mutex_lock(&mmvm_lock);

  struct vm_rg_struct rgnode;
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  if (cur_vma == NULL)
  {
    pthread_mutex_unlock(&mmvm_lock);
    return -1;
  }

  if (get_free_vmrg_area(caller, vmaid, size, &rgnode) == 0)
  {
    caller->mm->symrgtbl[rgid].rg_start = rgnode.rg_start;
    caller->mm->symrgtbl[rgid].rg_end = rgnode.rg_end;
    *alloc_addr = rgnode.rg_start;

    pthread_mutex_unlock(&mmvm_lock);
    return 0;
  }

  int inc_sz = PAGING_PAGE_ALIGNSZ(size);
  struct sc_regs regs = {.a1 = SYSMEM_INC_OP, .a2 = vmaid, .a3 = inc_sz};

  if (syscall(caller, 17, &regs) < 0)
  {
    pthread_mutex_unlock(&mmvm_lock);
    return -1;
  }

  if (get_free_vmrg_area(caller, vmaid, size, &rgnode) != 0)
  {
    pthread_mutex_unlock(&mmvm_lock);
    return -1;
  }

  caller->mm->symrgtbl[rgid].rg_start = rgnode.rg_start;
  caller->mm->symrgtbl[rgid].rg_end = rgnode.rg_end;
  *alloc_addr = rgnode.rg_start;

  pthread_mutex_unlock(&mmvm_lock);
  return 0;
}

/*__free - remove a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *
 */
int __free(struct pcb_t *caller, int vmaid, int rgid)
{
  if (rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
    return -1;

  struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);
  if (currg == NULL || currg->rg_start == -1)
    return -1;

  struct vm_rg_struct *new_region = malloc(sizeof(struct vm_rg_struct));
  if (new_region == NULL)
    return -1;

  new_region->rg_start = currg->rg_start;
  new_region->rg_end = currg->rg_end;
  new_region->rg_next = NULL;

  // Free all physical frames and clear page table entries for this region
  int start_pgn = PAGING_PGN(new_region->rg_start);
  int end_pgn = PAGING_PGN((new_region->rg_end - 1));
  for (int pgn = start_pgn; pgn <= end_pgn; ++pgn)
  {
    uint32_t pte = caller->mm->pgd[pgn];
    if (PAGING_PAGE_PRESENT(pte))
    {
      int fpn = PAGING_PTE_FPN(pte);
      MEMPHY_put_freefp(caller->mram, fpn);
      caller->mm->pgd[pgn] = 0;
    }
  }

  if (enlist_vm_freerg_list(caller->mm, new_region) != 0)
  {
    free(new_region);
    return -1;
  }

  currg->rg_start = -1;
  currg->rg_end = -1;

  return 0;
}

/*liballoc - PAGING-based allocate a region memory
 *@proc:  Process executing the instruction
 *@size: allocated size
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int liballoc(struct pcb_t *proc, uint32_t size, uint32_t reg_index)
{
  int addr;
  int ret = __alloc(proc, 0, reg_index, size, &addr);
  if (ret != 0)
    return -1;

#ifdef IODUMP
  printf("===== PHYSICAL MEMORY AFTER ALLOCATION =====\n");
  printf("PID=%d - Region=%d - Address=%08x - Size=%d byte\n", proc->pid, reg_index, addr, size);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1);
#endif
#endif

  return 0;
}

/*libfree - PAGING-based free a region memory
 *@proc: Process executing the instruction
 *@size: allocated size
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */

int libfree(struct pcb_t *proc, uint32_t reg_index)
{
  int ret = __free(proc, 0, reg_index);
  if (ret != 0)
    return -1;

#ifdef IODUMP
  printf("===== PHYSICAL MEMORY AFTER DEALLOCATION =====\n");
  printf("PID=%d - Region=%d\n", proc->pid, reg_index);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1);
#endif
#endif

  return ret;
}

/*pg_getpage - get the page in ram
 *@mm: memory region
 *@pagenum: PGN
 *@framenum: return FPN
 *@caller: caller
 *
 */
int pg_getpage(struct mm_struct *mm, int pgn, int *fpn, struct pcb_t *caller)
{
  if (pgn < 0 || pgn >= PAGING_MAX_PGN)
    return -1;

  uint32_t pte = mm->pgd[pgn];

  if (!PAGING_PAGE_PRESENT(pte))
  {
    int vicpgn, swpfpn, vicfpn;
    uint32_t vicpte;

    int tgtfpn = PAGING_PTE_SWP(pte);

    if (find_victim_page(caller->mm, &vicpgn) < 0)
      return -1;

    vicpte = mm->pgd[vicpgn];
    vicfpn = PAGING_PTE_FPN(vicpte);

    if (MEMPHY_get_freefp(caller->active_mswp, &swpfpn) < 0)
      return -1;

    struct sc_regs regs = {.a1 = SYSMEM_SWP_OP, .a2 = vicfpn, .a3 = swpfpn};
    if (syscall(caller, 17, &regs) < 0)
      return -1;

    pte_set_swap(&mm->pgd[vicpgn], 0, swpfpn);

    regs.a1 = SYSMEM_SWP_OP;
    regs.a2 = tgtfpn;
    regs.a3 = vicfpn;
    if (syscall(caller, 17, &regs) < 0)
      return -1;

    pte_set_fpn(&mm->pgd[pgn], vicfpn);
    enlist_pgn_node(&caller->mm->fifo_pgn, pgn);
  }

  *fpn = PAGING_FPN(mm->pgd[pgn]);
  return 0;
}

/*pg_getval - read value at given offset
 *@mm: memory region
 *@addr: virtual address to acess
 *@value: value
 *
 */
int pg_getval(struct mm_struct *mm, int addr, BYTE *data, struct pcb_t *caller)
{
  int pgn = PAGING_PGN(addr);
  if (pgn < 0 || pgn >= PAGING_MAX_PGN)
    return -1;

  int off = PAGING_OFFST(addr);
  int fpn;

  if (pg_getpage(mm, pgn, &fpn, caller) != 0)
    return -1;

  int phyaddr = fpn * PAGING_PAGESZ + off;
  struct sc_regs regs;
  regs.a1 = SYSMEM_IO_READ;
  regs.a2 = phyaddr;
  int ret = syscall(caller, 17, &regs);
  if (ret < 0)
    return -1;

  *data = (BYTE)regs.a3;

  return 0;
}

/*pg_setval - write value to given offset
 *@mm: memory region
 *@addr: virtual address to acess
 *@value: value
 *
 */
int pg_setval(struct mm_struct *mm, int addr, BYTE value, struct pcb_t *caller)
{
  int pgn = PAGING_PGN(addr);
  if (pgn < 0 || pgn >= PAGING_MAX_PGN)
    return -1;

  int off = PAGING_OFFST(addr);
  int fpn;

  if (pg_getpage(mm, pgn, &fpn, caller) != 0)
    return -1;

  int phyaddr = fpn * PAGING_PAGESZ + off;
  struct sc_regs regs;
  regs.a1 = SYSMEM_IO_WRITE;
  regs.a2 = phyaddr;
  regs.a3 = value;
  int ret = syscall(caller, 17, &regs);
  if (ret < 0)
    return -1;

  return 0;
}

/*__read - read value in region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@offset: offset to acess in memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *
 */
int __read(struct pcb_t *caller, int vmaid, int rgid, int offset, BYTE *data)
{
  struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  if (currg == NULL || cur_vma == NULL)
    return -1;

  pg_getval(caller->mm, currg->rg_start + offset, data, caller);

  return 0;
}

/*libread - PAGING-based read a region memory */
int libread(
    struct pcb_t *proc,
    uint32_t source,
    uint32_t offset,
    uint32_t *destination)
{
  BYTE data;
  int val = __read(proc, 0, source, offset, &data);

  *destination = (uint32_t)data;
#ifdef IODUMP
  printf("===== PHYSICAL MEMORY AFTER READING =====\n");
  printf("read region=%d offset=%d value=%d\n", source, offset, data);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1);
#endif
  MEMPHY_dump(proc->mram);
#endif

  return val;
}

/*__write - write a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@offset: offset to acess in memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *
 */
int __write(struct pcb_t *caller, int vmaid, int rgid, int offset, BYTE value)
{
  struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  if (currg == NULL || cur_vma == NULL)
    return -1;

  pg_setval(caller->mm, currg->rg_start + offset, value, caller);

  return 0;
}

/*libwrite - PAGING-based write a region memory */
int libwrite(
    struct pcb_t *proc,
    BYTE data,
    uint32_t destination,
    uint32_t offset)
{
  int val = __write(proc, 0, destination, offset, data);
#ifdef IODUMP
  printf("===== PHYSICAL MEMORY AFTER WRITING =====\n");
  printf("write region=%d offset=%d value=%d\n", destination, offset, data);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1);
#endif
  MEMPHY_dump(proc->mram);
#endif

  return val;
}

/*free_pcb_memphy - collect all memphy of pcb
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 */
int free_pcb_memph(struct pcb_t *caller)
{
  int pagenum, fpn;
  uint32_t pte;

  for (pagenum = 0; pagenum < PAGING_MAX_PGN; pagenum++)
  {
    pte = caller->mm->pgd[pagenum];

    if (!PAGING_PAGE_PRESENT(pte))
    {
      fpn = PAGING_PTE_FPN(pte);
      MEMPHY_put_freefp(caller->mram, fpn);
    }
    else
    {
      fpn = PAGING_PTE_SWP(pte);
      MEMPHY_put_freefp(caller->active_mswp, fpn);
    }
  }

  return 0;
}

/* find_victim_page - Find a victim page for replacement
 * @mm: Memory region
 * @retpgn: Pointer to store the victim page number
 */
int find_victim_page(struct mm_struct *mm, int *retpgn)
{
  struct pgn_t *pg = mm->fifo_pgn;

  if (pg == NULL)
    return -1;

  struct pgn_t *pg_prev = NULL;
  while (pg->pg_next != NULL)
  {
    pg_prev = pg;
    pg = pg->pg_next;
  }

  *retpgn = pg->pgn;
  if (pg_prev != NULL)
    pg_prev->pg_next = NULL;
  else
    mm->fifo_pgn = NULL;
  free(pg);

  return 0;
}

/*get_free_vmrg_area - get a free vm region
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@size: allocated size
 *
 */
int get_free_vmrg_area(struct pcb_t *caller, int vmaid, int size, struct vm_rg_struct *newrg)
{
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  struct vm_rg_struct *rgit = cur_vma->vm_freerg_list;

  if (rgit == NULL)
    return -1;

  newrg->rg_start = newrg->rg_end = -1;

  while (rgit != NULL)
  {
    if (rgit->rg_start + size <= rgit->rg_end)
    {
      newrg->rg_start = rgit->rg_start;
      newrg->rg_end = rgit->rg_start + size;

      if (rgit->rg_start + size < rgit->rg_end)
      {
        rgit->rg_start += size;
      }
      else
      {
        cur_vma->vm_freerg_list = NULL;
        free(rgit);
      }
      break;
    }
    rgit = rgit->rg_next;
  }

  return (newrg->rg_start == -1 && newrg->rg_end == -1) ? -1 : 0;
}