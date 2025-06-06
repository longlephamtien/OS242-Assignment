#include "mm.h"
#include <stdlib.h>
#include <stdio.h>

/*
 * init_pte - Initialize PTE entry
 */
int init_pte(uint32_t *pte,
             int pre,    // present
             int fpn,    // FPN
             int drt,    // dirty
             int swp,    // swap
             int swptyp, // swap type
             int swpoff) // swap offset
{
  if (pre != 0)
  {
    if (swp == 0)
    { // Non swap ~ page online
      if (fpn == 0)
        return -1; // Invalid setting

      SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
      CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);
      CLRBIT(*pte, PAGING_PTE_DIRTY_MASK);
      SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);
    }
    else
    { // Page swapped
      SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
      SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);
      CLRBIT(*pte, PAGING_PTE_DIRTY_MASK);
      SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
      SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);
    }
  }

  return 0;
}

/*
 * pte_set_swap - Set PTE entry for swapped page
 * @pte    : target page table entry (PTE)
 * @swptyp : swap type
 * @swpoff : swap offset
 */
int pte_set_swap(uint32_t *pte, int swptyp, int swpoff)
{
  SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
  SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);

  SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
  SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);

  return 0;
}

/*
 * pte_set_swap - Set PTE entry for on-line page
 * @pte   : target page table entry (PTE)
 * @fpn   : frame page number (FPN)
 */
int pte_set_fpn(uint32_t *pte, int fpn)
{
  SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
  CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);

  SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);

  return 0;
}

/*
 * vmap_page_range - map a range of page at aligned address
 */
int vmap_page_range(struct pcb_t *caller,           // process call
                    int addr,                       // start address which is aligned to pagesz
                    int pgnum,                      // num of mapping page
                    struct framephy_struct *frames, // list of the mapped frames
                    struct vm_rg_struct *ret_rg)    // return mapped region, the real mapped fp
{                                                   // no guarantee all given pages are mapped
  struct framephy_struct *fpit = frames;
  int pgit = 0;
  int pgn = PAGING_PGN(addr);
  /* TODO: update the rg_end and rg_start of ret_rg
  //ret_rg->rg_end =  ....
  //ret_rg->rg_start = ...
  //ret_rg->vmaid = ...
  */
  ret_rg->rg_start = addr;
  ret_rg->rg_end = addr + pgnum * PAGING_PAGESZ;

  /* TODO map range of frame to address space
   *      [addr to addr + pgnum*PAGING_PAGESZ
   *      in page table caller->mm->pgd[]
   */
  // enlist_vm_rg_node(&caller->mm->mmap, ret_rg);
  for (; pgit < pgnum; pgit++)
  {
    if (fpit == NULL)
      break;
    int curpgn = pgn + pgit;
    int curfpn = fpit->fpn;

    pte_set_fpn(&caller->mm->pgd[curpgn], curfpn);
    fpit = fpit->fp_next;
    // PAGING_PAGE_PRESENT(caller->mm->pgd[curpgn]);
    /* Tracking for later page replacement activities (if needed)
     * Enqueue new usage page */
    enlist_pgn_node(&caller->mm->fifo_pgn, curpgn);
  }

  return 0;
}

/*
 * alloc_pages_range - Allocate requested pages in RAM
 */
int alloc_pages_range(struct pcb_t *caller, int req_pgnum, struct framephy_struct **frm_lst)
{
  if (req_pgnum * PAGING_PAGESZ > caller->mram->maxsz)
  {
    return -3000;
  }

  int used_frames = 0;
  for (int i = 0; i < PAGING_MAX_PGN; i++)
  {
    if (PAGING_PAGE_PRESENT(caller->mm->pgd[i]))
    {
      used_frames++;
    }
  }
  if ((used_frames + req_pgnum) * PAGING_PAGESZ > caller->mram->maxsz)
  {
    return -3000;
  }

  int pgit, fpn;
  struct framephy_struct *newfp_str = NULL;

  for (pgit = 0; pgit < req_pgnum; pgit++)
  {
    newfp_str = (struct framephy_struct *)malloc(sizeof(struct framephy_struct));
    if (newfp_str == NULL)
    {
      // Free all previously allocated frames to prevent memory leaks
      struct framephy_struct *freefp_str;
      while (*frm_lst != NULL)
      {
        freefp_str = *frm_lst;
        *frm_lst = (*frm_lst)->fp_next;
        free(freefp_str);
      }
      return -3000;
    }

    if (MEMPHY_get_freefp(caller->mram, &fpn) == 0)
    {
      newfp_str->fpn = fpn;
    }
    else
    {
      // Handle insufficient frames by swapping
      int vicpgn, swpfpn;
      if (find_victim_page(caller->mm, &vicpgn) == -1 ||
          MEMPHY_get_freefp(caller->active_mswp, &swpfpn) == -1)
      {
        // Free all previously allocated frames to prevent memory leaks
        struct framephy_struct *freefp_str;
        while (*frm_lst != NULL)
        {
          freefp_str = *frm_lst;
          *frm_lst = (*frm_lst)->fp_next;
          free(freefp_str);
        }
        free(newfp_str); // Free the current frame structure
        return -3000;    // Out of memory
      }

      uint32_t vicpte = caller->mm->pgd[vicpgn];
      int vicfpn = PAGING_PTE_FPN(vicpte);
      __swap_cp_page(caller->mram, vicfpn, caller->active_mswp, swpfpn);
      pte_set_swap(&caller->mm->pgd[vicpgn], 0, swpfpn);
    }

    newfp_str->fp_next = *frm_lst;
    *frm_lst = newfp_str;
  }

  return 0;
}

/*
 * vm_map_ram - Map VM area to RAM
 */
int vm_map_ram(struct pcb_t *caller, int astart, int aend, int mapstart, int incpgnum, struct vm_rg_struct *ret_rg)
{
  struct framephy_struct *frm_lst = NULL;
  int ret_alloc = alloc_pages_range(caller, incpgnum, &frm_lst);

  if (ret_alloc < 0 && ret_alloc != -3000)
    return -1;

  if (ret_alloc == -3000)
  {
#ifdef MMDBG
    printf("OOM: vm_map_ram out of memory\n");
#endif
    return -1;
  }

  if (vmap_page_range(caller, mapstart, incpgnum, frm_lst, ret_rg) < 0)
  {
    // Free allocated frames in case of failure
    struct framephy_struct *freefp_str;
    while (frm_lst != NULL)
    {
      freefp_str = frm_lst;
      frm_lst = frm_lst->fp_next;
      free(freefp_str);
    }
    return -1;
  }

  // Free the frame list after successful mapping
  struct framephy_struct *freefp_str;
  while (frm_lst != NULL)
  {
    freefp_str = frm_lst;
    frm_lst = frm_lst->fp_next;
    free(freefp_str);
  }

  return 0;
}

/*
 * __swap_cp_page - Swap content between source and destination frames
 */
int __swap_cp_page(struct memphy_struct *mpsrc, int srcfpn, struct memphy_struct *mpdst, int dstfpn)
{
  for (int cellidx = 0; cellidx < PAGING_PAGESZ; cellidx++)
  {
    int addrsrc = srcfpn * PAGING_PAGESZ + cellidx;
    int addrdst = dstfpn * PAGING_PAGESZ + cellidx;

    BYTE data;
    MEMPHY_read(mpsrc, addrsrc, &data);
    MEMPHY_write(mpdst, addrdst, data);
  }

  return 0;
}

/*
 * init_mm - Initialize an empty Memory Management instance
 */
int init_mm(struct mm_struct *mm, struct pcb_t *caller)
{
  struct vm_area_struct *vma0 = malloc(sizeof(struct vm_area_struct));
  if (vma0 == NULL)
    return -1;

  mm->pgd = malloc(PAGING_MAX_PGN * sizeof(uint32_t));
  if (mm->pgd == NULL)
  {
    free(vma0);
    return -1;
  }

  vma0->vm_id = 0;
  vma0->vm_start = 0;
  vma0->vm_end = vma0->vm_start;
  vma0->sbrk = vma0->vm_start;

  struct vm_rg_struct *first_rg = init_vm_rg(vma0->vm_start, vma0->vm_end);
  if (first_rg == NULL)
  {
    free(vma0);
    free(mm->pgd);
    return -1;
  }

  enlist_vm_rg_node(&vma0->vm_freerg_list, first_rg);

  vma0->vm_next = NULL;
  vma0->vm_mm = mm;
  mm->mmap = vma0;

  return 0;
}

/*
 * init_vm_rg - Initialize a VM region
 */
struct vm_rg_struct *init_vm_rg(int rg_start, int rg_end)
{
  struct vm_rg_struct *rgnode = malloc(sizeof(struct vm_rg_struct));
  if (rgnode == NULL)
    return NULL;

  rgnode->rg_start = rg_start;
  rgnode->rg_end = rg_end;
  rgnode->rg_next = NULL;

  return rgnode;
}

int enlist_vm_rg_node(struct vm_rg_struct **rglist, struct vm_rg_struct *rgnode)
{
  rgnode->rg_next = *rglist;
  *rglist = rgnode;

  return 0;
}

int enlist_pgn_node(struct pgn_t **plist, int pgn)
{
  struct pgn_t *pnode = malloc(sizeof(struct pgn_t));

  pnode->pgn = pgn;
  pnode->pg_next = *plist;
  *plist = pnode;

  return 0;
}

int print_list_fp(struct framephy_struct *ifp)
{
  struct framephy_struct *fp = ifp;

  printf("print_list_fp: ");
  if (fp == NULL)
  {
    printf("NULL list\n");
    return -1;
  }
  printf("\n");
  while (fp != NULL)
  {
    printf("fp[%d]\n", fp->fpn);
    fp = fp->fp_next;
  }
  printf("\n");
  return 0;
}

int print_list_rg(struct vm_rg_struct *irg)
{
  struct vm_rg_struct *rg = irg;

  printf("print_list_rg: ");
  if (rg == NULL)
  {
    printf("NULL list\n");
    return -1;
  }
  printf("\n");
  while (rg != NULL)
  {
    printf("rg[%ld->%ld]\n", rg->rg_start, rg->rg_end);
    rg = rg->rg_next;
  }
  printf("\n");
  return 0;
}

int print_list_vma(struct vm_area_struct *ivma)
{
  struct vm_area_struct *vma = ivma;

  printf("print_list_vma: ");
  if (vma == NULL)
  {
    printf("NULL list\n");
    return -1;
  }
  printf("\n");
  while (vma != NULL)
  {
    printf("va[%ld->%ld]\n", vma->vm_start, vma->vm_end);
    vma = vma->vm_next;
  }
  printf("\n");
  return 0;
}

int print_list_pgn(struct pgn_t *ip)
{
  printf("print_list_pgn: ");
  if (ip == NULL)
  {
    printf("NULL list\n");
    return -1;
  }
  printf("\n");
  while (ip != NULL)
  {
    printf("va[%d]-\n", ip->pgn);
    ip = ip->pg_next;
  }
  printf("n");
  return 0;
}

int print_pgtbl(struct pcb_t *caller, uint32_t start, uint32_t end)
{
  int pgn_start, pgn_end;
  int pgit;

  if (end == -1)
  {
    pgn_start = 0;
    struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, 0);
    end = cur_vma->vm_end;
    //  printf ("end is : %d\n", end);
  }
  pgn_start = PAGING_PGN(start);
  pgn_end = PAGING_PGN(end);

  printf("print_pgtbl: %d - %d", start, end);
  if (caller == NULL)
  {
    printf("NULL caller\n");
    return -1;
  }
  printf("\n");

  for (pgit = pgn_start; pgit < pgn_end; pgit++)
  {
    printf("%08ld: %08x\n", pgit * sizeof(uint32_t), caller->mm->pgd[pgit]);
  }
  for (pgit = pgn_start; pgit < pgn_end; pgit++)
  {
    printf("Page Number: %d -> Frame Number: %d\n", pgit, PAGING_PTE_FPN(caller->mm->pgd[pgit]));
  }
  printf("================================================================\n");
  return 0;
}