// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
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
#define GET_BUCKETID(x) (x % NBUCKET)
#define GET_UNUSEDBUF(x) (NBUF-x)

struct {
  struct spinlock lock;
  struct buf buf[NBUF];
  uint rest;
  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
} bcache;

struct {
  struct spinlock lock;
  struct buf * head;
} bbucket[NBUCKET];

void
binit(void)
{
  initlock(&bcache.lock, "bcache");
  for(int i = 0; i < NBUF; ++i)
    bcache.buf[i].prev = bcache.buf[i].next = 0;
  bcache.rest = NBUF;

  char name[12];
  for(int i = 0; i < NBUCKET; ++i){
    snprintf(name, 12, "bbucket_%d", i);
    initlock(&bbucket[i].lock, name);
    bbucket[i].head = 0;
  }
}

static struct buf*
_bget(uint dev, uint blockno, uint bid){
  struct buf * b, * empty = 0;
    // Is the block already cached?
  for(b = bbucket[bid].head; b; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      ++b->refcnt;
      release(&bbucket[bid].lock);
      acquiresleep(&b->lock);
      return b;
    }else if(b->refcnt == 0)
      empty = b;
  }
  
  if(empty){
    empty->dev = dev;
    empty->blockno = blockno;
    empty->valid = 0;
    empty->refcnt = 1;
    release(&bbucket[bid].lock);
    acquiresleep(&empty->lock);
    return empty;
  }
  return 0;
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  uint bid = GET_BUCKETID(blockno);

  acquire(&bbucket[bid].lock);
  if((b = _bget(dev, blockno, bid)) != 0)
    return b;

  release(&bbucket[bid].lock);
  acquire(&bcache.lock);

  
  // Not cached.
  if(bcache.rest != 0){
    b = &bcache.buf[GET_UNUSEDBUF(bcache.rest)];
    --bcache.rest;
  }else{
    for(int i = 1; i < NBUCKET; ++i){
      acquire(&bbucket[(bid+i)%NBUCKET].lock);
      for(b = bbucket[(bid+i)%NBUCKET].head; b; b = b->next){
        if(b->refcnt == 0) {
          if(b->prev)
            b->prev->next = b->next;
          else
            bbucket[(bid+i)%NBUCKET].head = b->next;
          if(b->next)
            b->next->prev = b->prev;
          release(&bbucket[(bid+i)%NBUCKET].lock);
          goto bget_break;
        }
      }
      release(&bbucket[(bid+i)%NBUCKET].lock);
    }
  }

bget_break:
  acquire(&bbucket[bid].lock);
  if(b){
    b->next = bbucket[bid].head;
    b->prev = 0;
    bbucket[bid].head = b;
  }
  if((b = _bget(dev, blockno, bid)) != 0){
    release(&bcache.lock);
    return b;
  }
  panic("bget: no buffers");
}

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

  uint bid = GET_BUCKETID(b->blockno);
  releasesleep(&b->lock);

  acquire(&bbucket[bid].lock);
  --b->refcnt;
  release(&bbucket[bid].lock);
}

void
bpin(struct buf *b) {
  uint bid = GET_BUCKETID(b->blockno);
  acquire(&bbucket[bid].lock);
  ++b->refcnt;
  release(&bbucket[bid].lock);
}

void
bunpin(struct buf *b) {
  uint bid = GET_BUCKETID(b->blockno);
  acquire(&bbucket[bid].lock);
  --b->refcnt;
  release(&bbucket[bid].lock);
}


