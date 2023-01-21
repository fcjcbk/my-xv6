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

// #define PAGE_MAX (PHYSTOP - KERNBASE) / PGSIZE
#define PAGE_MAX PHYSTOP / PGSIZE + 1
struct {
  struct spinlock lock;
  struct spinlock count_lock;
  struct run *freelist;
  int ref_count[PAGE_MAX];

} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&kmem.count_lock, "p_count");
  memset(kmem.ref_count, 0, sizeof(kmem.ref_count));
  for(int i = 0; i < PAGE_MAX; i++){
    kmem.ref_count[i] = 1;
  }
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
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

  // acquire(&kmem.lock);
  acquire(&kmem.count_lock);
  kmem.ref_count[(uint64)pa / PGSIZE]--;
  if(kmem.ref_count[(uint64)pa / PGSIZE] == 0){
    // Fill with junk to catch dangling refs.
    memset(pa, 1, PGSIZE);

    r = (struct run*)pa;

    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
  }
  release(&kmem.count_lock);
  // release(&kmem.lock);

  
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r){
    kmem.freelist = r->next;
  }
  release(&kmem.lock);

  if(r){
    memset((char*)r, 5, PGSIZE); // fill with junk
    // acquire(&kmem.count_lock);
    kmem.ref_count[(uint64)r / PGSIZE] = 1;
    // release(&kmem.count_lock);
  }
  return (void*)r;
}

void add_count(void * pa){
  acquire(&kmem.count_lock);
  kmem.ref_count[(uint64)pa / PGSIZE]++;
  release(&kmem.count_lock);

}

uint64 copy_page_if_count(uint64 pa){
  acquire(&kmem.count_lock);
  if(kmem.ref_count[(uint64)pa / PGSIZE] > 1){
    char* mem;
    if((mem = kalloc()) == 0){
      release(&kmem.count_lock);
      return 0;
    }
    memmove(mem, (char*)pa, PGSIZE);
    kmem.ref_count[(uint64)pa / PGSIZE]--;
    pa = (uint64)mem;
  }
  release(&kmem.count_lock);
  return pa;
}