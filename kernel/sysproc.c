#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "date.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;


  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}


#ifdef LAB_PGTBL
int
sys_pgaccess(void)
{
  // lab pgtbl: your code here.
  int page_nums;
  uint64 p;
  uint64 res;
  pagetable_t pagetable = myproc()->pagetable;
  if(argaddr(0, &p) < 0){
    return -1;
  }
  if(argint(1, &page_nums) < 0){
    return -1;
  }
  if(argaddr(2, &res) < 0){
    return -1;
  }
  if(page_nums > 63){
    return -1;
  }

  int index = 0;
  uint64 buf = 0;
  pte_t* pte = walk(pagetable, p, 0);
  
  while(index < page_nums){

    if((pte[index] & PTE_V) && (pte[index] & PTE_A)){
      buf |= (1 << index);
      pte[index] &= (~PTE_A);
    }
    

    index++;
  }

  if(copyout(pagetable, res, (char*)&buf, sizeof(buf)) < 0){
    return -1;
  }
  return 0;
}
#endif

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
