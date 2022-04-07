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

// lab8
#define BUCKET_SIZE 13 
#define BUF_SIZE 8

struct Bucket{
  struct spinlock lock;
  struct buf buf[BUF_SIZE];

  struct buf head;
};

struct Bucket bucket[BUCKET_SIZE];

void
binit(void)
{
  struct buf *b;

  // lab8

  for(int i = 0; i < BUCKET_SIZE; ++i){ 
	  initlock(&bucket[i].lock, "bucket");
	  bucket[i].head.prev = &bucket[i].head;
	  bucket[i].head.next = &bucket[i].head;
	  for (b = bucket[i].buf; b < bucket[i].buf + BUF_SIZE; b++){
		  b->next = bucket[i].head.next;
		  b->prev = &bucket[i].head;
		  initsleeplock(&b->lock, "buffer");
		  b->refcnt = 0;
		  bucket[i].head.next->prev = b;
		  bucket[i].head.next = b;
	  }
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  // lab8 
  uint i = blockno % BUCKET_SIZE;
  acquire(&bucket[i].lock);

  b = &bucket[i].head;

  do {
	if (b->dev == dev && b->blockno == blockno)
	{
		b->refcnt++;
		release(&bucket[i].lock);
		acquiresleep(&b->lock);
		return b;
	}
	b = b->next;
  } while (b != &bucket[i].head);

  b = &bucket[i].head;

  do {
	  if (b->refcnt == 0)
	  {
		b->dev = dev;
		b->blockno = blockno;
		b->refcnt = 1;
		b->valid = 0;
		release(&bucket[i].lock);
		acquiresleep(&b->lock);
		return b;
	  }
	  b = b->next;
  } while (b != &bucket[i].head);

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

  releasesleep(&b->lock);

  uint i = b->blockno % BUCKET_SIZE;

  acquire(&bucket[i].lock);
  b->refcnt--;
  release(&bucket[i].lock);

}

void
bpin(struct buf *b) {
  uint i = b->blockno % BUCKET_SIZE;
  acquire(&bucket[i].lock);
  b->refcnt++;
  release(&bucket[i].lock);
  //acquire(&bcache.lock);
  //b->refcnt++;
  //release(&bcache.lock);
}

void
bunpin(struct buf *b) {
  uint i = b->blockno % BUCKET_SIZE;
  acquire(&bucket[i].lock);
  b->refcnt--;
  release(&bucket[i].lock);
  //acquire(&bcache.lock);
  //b->refcnt--;
  //release(&bcache.lock);
}


