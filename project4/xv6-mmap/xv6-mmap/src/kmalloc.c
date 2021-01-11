//the goal is to give xv6 kernel the ability to allocate memory for 
//kernel data structures without wasting space 
#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"

typedef long Align;

union header {
  struct {
    union header *ptr;
    uint size;
  } s;
  Align x;
};

typedef union header Header;

static Header base;
static Header *freep;

void 
kmfree(void *addr)   //add system call for testing 
{
  Header *bp, *p;

  bp = (Header*)addr - 1;
  for(p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr)
    if(p >= p->s.ptr && (bp > p || bp < p->s.ptr))
      break;
  if(bp + bp->s.size == p->s.ptr){
    bp->s.size += p->s.ptr->s.size;
    bp->s.ptr = p->s.ptr->s.ptr;
  } else
    bp->s.ptr = p->s.ptr;
  if(p + p->s.size == bp){
    p->s.size += bp->s.size;
    p->s.ptr = bp->s.ptr;
  } else
    p->s.ptr = bp;
  freep = p;
}

static Header*
morecore()
{
  char *p;
  Header *hp;
  p = kalloc(); //receive a free page to refill the free chunks

  if(!p)
    return 0;
    
  hp = (Header*)p;
  hp->s.size = 4096;
  kmfree((void*)(hp + 1));
  return freep;
}

void* 
kmalloc(uint nbytes)
{ 
  //panic to check if the request is greater than a page in length
  if (nbytes > 4096)
    panic("kmalloc does not allocate more than 4096 bytes.");
  
  //copy the code from malloc() in umalloc.c 
  Header *p, *prevp;
  uint nunits;

  nunits = (nbytes + sizeof(Header) - 1)/sizeof(Header) + 1;
  if((prevp = freep) == 0){
    base.s.ptr = freep = prevp = &base;
    base.s.size = 0;
  }
  for(p = prevp->s.ptr; ; prevp = p, p = p->s.ptr){
    if(p->s.size >= nunits){
      if(p->s.size == nunits)
        prevp->s.ptr = p->s.ptr;
      else {
        p->s.size -= nunits;
        p += p->s.size;
        p->s.size = nunits;
      }
      freep = prevp;
      return (void*)(p + 1);
    }
    if(p == freep)
      if((p = morecore()) == 0)
        return 0;
  }
}
