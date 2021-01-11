#include "param.h"
#include "types.h"
#include "param.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

/* mmap creats a new mapping for the calling process's address space
 * this function will round up to a page aligned address if needed
 * 
 * Inputs:  addr -  suggestion for starting address, mmap will
 *                  round up to page aligned addr if needed
 *                  (NULL --> place at any appropriate address)
 *          length- length of region to allocate (in bytes)
 *          prot -  protection level on mapped region
 *          flags - info flags for mapped region
 *          fd -    file descriptors
 *          offset- offset for a file-backed allocation 
 * 
 * Returns: starting address (page aligned) of mapped region
 * or (void*)-1 on failure
 */
 
void *mmap(void *addr, int length, int prot, int flags, int fd, int offset)
{   
    //get pointer to current process
    struct proc *curproc = myproc();
    int address = curproc->sz;
    
    if((curproc->sz = allocuvm(curproc->pgdir, address, address + length)) == 0)
        return (void*)-1;
   
    switchuvm(curproc);
    cprintf("After switchuvm\n");
    
    //List nodes are allocated using kmalloc (from kmalloc.c)
    //assigning memory to struct variable
    struct mmap_region *new_mmap_region = kmalloc(sizeof(struct mmap_region));
    cprintf("After kmalloc\n");

    //assign list data and meta data to the new region
    new_mmap_region->addr = (void*)PGROUNDDOWN((uint)address);
    new_mmap_region->length = length;
    new_mmap_region->fd = fd;
    new_mmap_region->offset = offset;
    new_mmap_region->next = 0;
    cprintf("After assigning mmap \n");
    
    // first region, save as head
    if(curproc->mmap_sz == 0) 
    {
        curproc->mmap_hd = new_mmap_region;
    }
    else  //add region to existing mapped_regions list
    {
        struct mmap_region *cursor = curproc->mmap_hd;
        while (cursor->next)
        {
            cursor = cursor->next;
        }

        cursor->next = new_mmap_region;
    }
    
    curproc->mmap_sz += 1;
    return new_mmap_region->addr;
}

/* munmap assumes that the address and length given will exactly
 * match an mmap node in our linked list (given that the node exits).
 * 
 * Inputs:  addr    - starting address of the region to unmap
 *          length  - length of the region to unmap
 * 
 * Returns: 0   - On success
 *          -1  - On failure
 */

int munmap(void *addr, uint length)
{
    struct proc *curproc = myproc();
    int address = curproc->sz;

    //if nothing has been allocated, there is nothing to munmap
    if (curproc->mmap_sz == 0)
    {
        return -1;
    }

    struct mmap_region *head = curproc->mmap_hd;
    struct mmap_region *itr = head;
    struct mmap_region *tmp; 
    int find = 0;

    if(head->addr == addr && head->length == length)
    {
        tmp = head;
        curproc->mmap_hd = head->next;
        find = 1;
    }
    else
    {
        while(itr->next)
        {
            if(itr->next->addr == addr && itr->next->length == length)
            {
                tmp = itr->next;
                itr->next = itr->next->next;
                find = 1;
                break;
            }
            itr = itr->next;
        }        
    }

    if(find)
    {   
        //deallocate the memory from the current process
        if((curproc->sz = deallocuvm(curproc->pgdir, address, address - length)) == 0)
        {
            free_mmap_regions(curproc->mmap_hd);
            return -1;
        }
        
        switchuvm(curproc);
        kmfree(tmp);   
        curproc->mmap_sz -= 1;
        return 0;  //return success
    }

    free_mmap_regions(curproc->mmap_hd);
    return -1;
}

// Recursively free mmap regions
void free_mmap_regions(struct mmap_region *mmap_region)
{
    if(mmap_region && mmap_region->next)
        free_mmap_regions(mmap_region->next);  
    if (mmap_region)
        kmfree(mmap_region);
}