// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk bbcache.locks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk bbcache.locks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKET 13

struct {
  struct spinlock evict_lock[NBUCKET];
  struct buf buf[NBUF];
  struct buf table[NBUCKET];
  struct spinlock locks[NBUCKET];
  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
} bcache;


static struct buf* bget(uint dev, uint blockno);


#define PATTERN "bcache_%d"


// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.


// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.

void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  int index = b->blockno % NBUCKET;

  acquire(&bcache.locks[index]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->lastuse = ticks;
  }
  release(&bcache.locks[index]);
}

static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  int index = blockno % NBUCKET;
  acquire(&bcache.locks[index]);

  // Is the block already cached?
  for(b = bcache.table[index].next; b != 0; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;

      release(&bcache.locks[index]);
      
      acquiresleep(&b->lock);
      return b;
    }
  }

  release(&bcache.locks[index]);

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.


  acquire(&bcache.evict_lock[index]);

  for(b = bcache.table[index].next; b != 0; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      acquire(&bcache.locks[index]);
      b->refcnt++;

      // printf("bget release lock %d\n", index);
      release(&bcache.locks[index]);
      release(&bcache.evict_lock[index]);   
      acquiresleep(&b->lock);
      return b;
    }
  }

  struct buf *prev = 0;
  int n_index = -1;

  for(int i = 0; i < NBUCKET; i++){
    acquire(&bcache.locks[i]);

    for(b = &bcache.table[i];  b->next != 0; b = b->next){
      if(b->next->refcnt == 0){
        if(n_index == -1){
          n_index = i;
          prev = b;
        }else if(b->next->lastuse < prev->lastuse){
          if(n_index != i){
            release(&bcache.locks[n_index]);
            n_index = i;
          }
          prev = b;
        }
      }
    }
    if(n_index != i){
      release(&bcache.locks[i]);
    }
  }
  if(prev == 0){
    panic("bget");
  }
  b = prev->next;
  if(n_index != index){
    prev->next = b->next;
    release(&bcache.locks[n_index]);
    
    acquire(&bcache.locks[index]);
    b -> next = bcache.table[index].next;
    bcache.table[index].next = b;
  }
  
  b->dev = dev;
  b->blockno = blockno;
  b->valid = 0;
  b->refcnt = 1;
  
  
  
  // printf("bget release lock %d\n", index);
  release(&bcache.locks[index]);
  
  // printf("bget release global lock\n");
  release(&bcache.evict_lock[index]);

  acquiresleep(&b->lock);
  return b;
}

void
binit(void)
{
  struct buf *b;

  for(int i = 0; i < NBUCKET; i++){
    char buffers[16];
    snprintf(buffers, 1, PATTERN, i);
    initlock(&bcache.locks[i], buffers);
    bcache.table[i].next = 0;
    initlock(&bcache.evict_lock[i], "bcache");
  }

  // Create linked list of buffers
  for(int i = 0; i < NBUF; i++){
    b = &bcache.buf[i];
    initsleeplock(&b->lock, "buffer");
    b->refcnt = 0;
    b->lastuse = 0;
    b->next = bcache.table[0].next;
    bcache.table[0].next = b;
  }
}

void
bpin(struct buf *b) {
  int index = b->blockno % NBUCKET;


  acquire(&bcache.locks[index]);

  b->refcnt++;


  release(&bcache.locks[index]);
}

void
bunpin(struct buf *b) {
  int index = b->blockno % NBUCKET;

  acquire(&bcache.locks[index]);
  b->refcnt--;
  release(&bcache.locks[index]);
}
