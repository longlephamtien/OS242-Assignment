#include "string.h"
#include "mm.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

/* get_vma_by_num - Get VM area by ID */
struct vm_area_struct *get_vma_by_num(struct mm_struct *mm, int vmaid)
{
  struct vm_area_struct *pvma = mm->mmap;

  if (pvma == NULL)
    return NULL;

  while (pvma != NULL && pvma->vm_id < vmaid)
  {
    pvma = pvma->vm_next;
  }

  return (pvma != NULL && pvma->vm_id == vmaid) ? pvma : NULL;
}

/* __mm_swap_page - Swap a page between RAM and swap space */
int __mm_swap_page(struct pcb_t *caller, int vicfpn, int swpfpn)
{
  return __swap_cp_page(caller->mram, vicfpn, caller->active_mswp, swpfpn);
}

/* get_vm_area_node_at_brk - Get VM area node at the break point */
struct vm_rg_struct *get_vm_area_node_at_brk(struct pcb_t *caller, int vmaid, int size, int alignedsz)
{
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  if (cur_vma == NULL)
    return NULL;

  struct vm_rg_struct *newrg = malloc(sizeof(struct vm_rg_struct));
  if (newrg == NULL)
    return NULL;

  newrg->rg_start = cur_vma->sbrk;
  newrg->rg_end = cur_vma->sbrk + alignedsz;
  newrg->rg_next = NULL;

  return newrg;
}

/* validate_overlap_vm_area - Validate overlap of VM area */
int validate_overlap_vm_area(struct pcb_t *caller, int vmaid, int vmastart, int vmaend)
{
  struct vm_area_struct *vma = get_vma_by_num(caller->mm, vmaid);
  if (vma == NULL)
    return -1;

  struct vm_rg_struct *rgit = vma->vm_freerg_list;
  while (rgit != NULL)
  {
    if (!(rgit->rg_end <= vmastart || rgit->rg_start >= vmaend))
      return -1; // Overlap detected
    rgit = rgit->rg_next;
  }

  return 0; // No overlap
}

/* inc_vma_limit - Increase VM area limits to reserve space */
int inc_vma_limit(struct pcb_t *caller, int vmaid, int inc_sz)
{
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  if (cur_vma == NULL)
    return -1;

  int inc_amt = PAGING_PAGE_ALIGNSZ(inc_sz);
  int incnumpage = inc_amt / PAGING_PAGESZ;

  struct vm_rg_struct *area = get_vm_area_node_at_brk(caller, vmaid, inc_sz, inc_amt);
  if (area == NULL)
    return -1;

  if (validate_overlap_vm_area(caller, vmaid, area->rg_start, area->rg_end) < 0)
  {
    free(area);
    return -1; // Overlap detected
  }

  int old_end = cur_vma->vm_end;
  cur_vma->vm_end += inc_sz;
  cur_vma->sbrk += inc_sz;

  struct vm_rg_struct *newrg = malloc(sizeof(struct vm_rg_struct));
  if (newrg == NULL)
  {
    free(area);
    return -1;
  }

  if (vm_map_ram(caller, area->rg_start, area->rg_end, old_end, incnumpage, newrg) < 0)
  {
    free(area);
    free(newrg);
    return -1; // Mapping failed
  }

  free(area);
  enlist_vm_rg_node(&cur_vma->vm_freerg_list, newrg);
  return 0;
}

// #endif
