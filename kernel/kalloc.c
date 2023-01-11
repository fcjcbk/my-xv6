// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct kmem{
  struct spinlock lock;
  struct run *freelist;
};

struct spinlock kmem_lock;

struct kmem kmems[NCPU];

#define pattern "kmem_%d"

void
kinit()
{
  initlock(&kmem_lock, "kmem_global");
  for (int i = 0;i < NCPU; i++) {
    char buf[16];
    snprintf(buf, 1, pattern, i);
    initlock(&kmems[i].lock, buf);
  }
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);

  int n = ((char*)pa_end - p) / PGSIZE;
  int i = 0;
  int count = 0;
  // printf("n = %d\n", n);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    if(count < n){
      count++;
    }else{
      count = 0;
      i++;
    }

    acquire(&kmems[i].lock);
    memset(p, 1, PGSIZE);
    struct run *r = (struct run*) p;
    r->next = kmems[i].freelist;
    kmems[i].freelist = r;
    release(&kmems[i].lock);
  }
  //   kfree(p);
  
  // for (int i = 0; i < NCPU; i++) {
  //   struct run *r;
  //   printf("i = %d\n", i);
  //   for (int j = 0; j < n - 1; j++) {
  //     acquire(&kmems[i].lock);
  //     if (p + PGSIZE > (char*) pa_end){
  //       break;
  //     }
  //     printf("j = %d\n", j);

  //     memset(p, 1, PGSIZE);
  //     r = (struct run*) p;
  //     r -> next = kmems[i].freelist;
  //     kmems[i].freelist->next = r;
  //     release(&kmems[i].lock);
  //     p += PGSIZE;
  //   }
  // }
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;
  push_off();

  int id = cpuid();
  
  acquire(&kmems[id].lock);
  
  r->next = kmems[id].freelist;
  kmems[id].freelist = r;
  
  release(&kmems[id].lock);
  
  pop_off();

}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  push_off();
  
  int id = cpuid();
  acquire(&kmems[id].lock);
  r = kmems[id].freelist;
  if (r) {
    kmems[id].freelist = r->next;
  }else{
    
    acquire(&kmem_lock);
    for (int i = 0; i < NCPU; i++) {
      if (i != id) {
        if (kmems[i].freelist) {
          r = kmems[i].freelist;
          kmems[i].freelist = kmems[i].freelist->next;
          break;
        }
      }
    }
    release(&kmem_lock);
  }
  release(&kmems[id].lock);
  
  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk

  pop_off();

  return (void*)r;
}
