#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "mman.h"
#include "proc.h"

#define NULL (mmap_region*)0

//helper functions
static void free_node(mmap_region*, mmap_region*);

void *mmap(void *addr, uint length, int prot, int flags, int fd, int offset)
{
  //get pointer to current process
  struct proc *curpro = myproc();
  uint oldsz = curpro->sz;
  uint newsz = curpro->sz + length;

  //check argument inputs 
  if (addr < (void*)0 || addr == (void*)KERNBASE || addr > (void*)KERNBASE || length < 1)
  {
    return (void*)-1;
  }

  curpro->sz = newsz;

  //List nodes are allocated using kmalloc (from kmalloc.c)
  //assigning memory to struct variable
  mmap_region* new_mapp_region = (mmap_region*)kmalloc(sizeof(mmap_region));
  if (new_mapp_region == NULL)
  {
    return (void*)-1;
  }

  //assign list data and meta data to the new region
  new_mapp_region->start_addr = (addr = (void*)(PGROUNDDOWN(oldsz) + MMAPBASE));
  new_mapp_region->length = length;
  new_mapp_region->prot = prot;
  new_mapp_region->region_type = flags;
  new_mapp_region->offset = offset;
  new_mapp_region->next = 0;

  //check the flags and file descriptor argument
  if (flags == MAP_ANONYMOUS)
  {
    if (fd != -1) //fd must be -1 
    {
      kmfree(new_mapp_region);
      return (void*)-1;
    }
   
  }
  else if (flags == MAP_FILE)
  {
    if (fd > -1)
    {
      if((fd=fdalloc(curpro->ofile[fd])) < 0)
        return (void*)-1;
      filedup(curpro->ofile[fd]);
      new_mapp_region->fd = fd;
    }
    else
    {
      kmfree(new_mapp_region);
      return (void*)-1;
    }
  }

  //handle first call to mmap
  if (curpro->nregions == 0)
  {
    curpro-> mmap_hd = new_mapp_region;
  }
  else //add region to an already existing mapped_regions list
  {
    mmap_region* itr = curpro->mmap_hd;
    while (itr->next != 0)
    {
      if (addr == itr->start_addr)
      {
        addr += PGROUNDDOWN(PGSIZE+itr->length);
        itr = curpro-> mmap_hd; //start over, we may overlap past regions
      }
      else if (addr == (void*)KERNBASE || addr > (void*)KERNBASE) //when run out of memory!
      {
        kmfree(new_mapp_region);
        return (void*)-1;
      }
      itr = itr->next;
    }
    // Catch the final node that isn't checked in the loop
    if (addr == itr->start_addr)
    {
      addr += PGROUNDDOWN(PGSIZE+itr->length);
    }

    //add new region to the end of our mmap_regions list
    itr->next = new_mapp_region;
  }

  //increase region count and retrun the new region's starting address
  curpro->nregions++;
  new_mapp_region->start_addr = addr;

  return new_mapp_region->start_addr;
}


int munmap(void *addr, uint length)
{
  struct proc *curpro = myproc();
  //check the addr and length
  if (addr == (void*)KERNBASE || addr > (void*)KERNBASE || length < 1)
  {
    return -1;
  }

  //if nothing has been allocated, no need to munmap
  if (curpro->nregions == 0)
  {
    return -1;
  }

  //travese our mmap dll 
  mmap_region *prev = curpro->mmap_hd;
  mmap_region *next = curpro->mmap_hd->next;
  int find = 0;

  //check the head
  if (curpro->mmap_hd->start_addr == addr && curpro->mmap_hd->length == length)
  {
    //deallocate the memory from current process
    curpro->sz = deallocuvm(curpro->pgdir, curpro->sz, curpro->sz - length);
    switchuvm(curpro);
    curpro->nregions--;  

    //close the file that we are mapping to
    if(curpro->mmap_hd->region_type == MAP_FILE)
    {
      fileclose(curpro->ofile[curpro->mmap_hd->fd]);
      curpro->ofile[curpro->mmap_hd->fd] = 0;
    }

    if(curpro->mmap_hd->next != 0)
    {
      find = curpro->mmap_hd->next->length;
      free_node(curpro->mmap_hd, 0);
      curpro->mmap_hd->length = find;
    }
    else
    {
      free_node(curpro->mmap_hd, 0);
    }

    //success
    return 0;
  }

  while(next != 0)
  {
    if (next->start_addr == addr && next->length == length)
    {
      //deallocate the memory from current process
      curpro->sz = deallocuvm(curpro->pgdir, curpro->sz, curpro->sz - length);
      switchuvm(curpro);
      curpro->nregions--;  
      
      // close the file we were mapping to
      if(next->region_type == MAP_FILE)
      {
        fileclose(curpro->ofile[next->fd]);
        curpro->ofile[next->fd] = 0;
      }

      //remove the node from our linked list
      find = next->next->length;
      free_node(next, prev);
      prev->next->length = find;
      
      //success
      return 0;
    }
    prev = next;
    next = prev->next;
  }

  return -1;
}

int msync (void* start_addr, uint length)
{
  struct proc *curproc = myproc();

  // If nothing has been allocated, no need to do msync
  if (curproc->nregions == 0)
  {
    return -1;
  }

  mmap_region *itr = curproc->mmap_hd;

  while(itr)
  {
    if(itr->start_addr == start_addr && itr->length == length)
    {
      //check the address was allocated
      pte_t* ret = walkpgdir(curproc->pgdir, start_addr, 0);
      if((uint)ret & PTE_D)
      {
        //do nothing
      }

      fileseek(curproc->ofile[itr->fd], itr->offset);
      filewrite(curproc->ofile[itr->fd], start_addr, length);
      return 0;
    }

    itr = itr->next;
  }
  
  return -1; //No match
}

 //helper function, free a node from linked list
static void free_node (mmap_region *node, mmap_region *prev)
{
  if (node == myproc()->mmap_hd)
  { 
        if(myproc()->mmap_hd->next != 0)
        {
           myproc()->mmap_hd= myproc()->mmap_hd->next;
        }
        else
        {
           myproc()->mmap_hd = 0;
        }
  }
  else
  {

      prev->next = node->next;
  }
  kmfree(node);
}

//helper function, free all elements of mmap linked list 
void free_mmap_regions()
{
  mmap_region* region = myproc()->mmap_hd;
  mmap_region* r;

  while (region != 0)
  {
    r = region;
    free_node(region, 0);
    region = r->next;
  }
}
