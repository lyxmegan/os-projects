#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mman.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;

void
pagefault_handler(struct trapframe *tf)
{
  struct proc *curproc = myproc();
  //control Register 2, holds the faulting page address.
  uint fault_addr = PGROUNDDOWN(rcr2()); 

  //debugging statements from the instruction
  cprintf("============in pagefault_handler============\n");
  cprintf("pid %d %s: trap %d err %d on cpu %d "
  "eip 0x%x addr 0x%x\n",
  curproc->pid, curproc->name, tf->trapno,
  tf->err, cpuid(), tf->eip, fault_addr);
 
  //if the faulting address belongs to a valid mmap region 
  int find = 0;
  mmap_region *head = curproc->mmap_hd;

  while(head)
  {
    if ((uint)(head->start_addr) <= fault_addr && (uint)(head->start_addr + head->length) > fault_addr)
    { 
      if ((head->prot & PROT_WRITE) || !(tf->err & T_ERR_PGFLT_W)) 
      {
        find = 1;
        break;
      }

      head = head->next;
    }
  }

  // mapping a single page around the faulting address, used the allocuvm()
  if (find == 1)
  {
    char *memory = kalloc();

    if(memory == 0)
    {
      goto error;
    }
    memset(memory, 0, PGSIZE);

    // if protection bits needed for mappages() 
    int permissions;
    if (head->prot == PROT_WRITE)
    {
      permissions = PTE_W|PTE_U; //give write permissions
    }
    else
    {
      permissions = PTE_U; //No write permissions
    }

    if(mappages(curproc->pgdir, (char*)fault_addr, PGSIZE, V2P(memory), permissions) < 0)
    {
      kfree(memory);
      goto error;
    }

    switchuvm(curproc);

    //when performing file-backed mmap, find the place we need in the
    //file and then read it into the memory location allocated above
    if (head->region_type == MAP_FILE)
    {
      if (curproc->ofile[head->fd])
      {
          fileseek(curproc->ofile[head->fd], head->offset);
          fileread(curproc->ofile[head->fd], memory, head->length);
          //Clear the dirty bit 
          pde_t* pde = &(myproc()->pgdir)[PDX(head->start_addr)];
          pte_t* pgtab = (pte_t*)P2V(PTE_ADDR(*pde));
          pte_t* pte = &pgtab[PTX(head->start_addr)];
          *pte &= ~PTE_D;
      }
    }
  }
  else  //Page fault on a non-allocated address
  {
    error:
      if((tf->cs&3) ==0 || myproc() == 0){
        // in kernel
        cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
                tf->trapno, cpuid(), tf->eip, rcr2());
        panic("trap");
      }
      // in user space
      cprintf("pid %d %s: trap %d err %d on cpu %d "
              "eip 0x%x addr 0x%x\n",
              myproc()->pid, myproc()->name, tf->trapno,
              tf->err, cpuid(), tf->eip, rcr2());
      myproc()->killed = 1;
  }
}

void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);

  initlock(&tickslock, "time");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

//PAGEBREAK: 41
void
trap(struct trapframe *tf)
{
  if(tf->trapno == T_SYSCALL){
    if(myproc()->killed)
      exit();
    myproc()->tf = tf;
    syscall();
    if(myproc()->killed)
      exit();
    return;
  }
  
  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
    }
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpuid(), tf->cs, tf->eip);
    lapiceoi();
    break;
  case T_PGFLT: //added pagefault check in the trap switch statement
    pagefault_handler(tf);
    break;
  //PAGEBREAK: 13
  default:
    if(myproc() == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
    myproc()->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  if(myproc() && myproc()->state == RUNNING &&
     tf->trapno == T_IRQ0+IRQ_TIMER)
    yield();

  // Check if the process has been killed since we yielded
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();
}
