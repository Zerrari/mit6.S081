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
#define BUF_SIZE 5

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  struct spinlock hash_lock[BUCKET_SIZE];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf* hash[BUCKET_SIZE];
} bcache;

struct Bucket{
  struct spinlock lock;
  struct buf buf[BUF_SIZE];

  struct buf* head;
};

struct Bucket bucket[BUCKET_SIZE];

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");

  // lab8
  for (int i = 0; i < BUCKET_SIZE; ++i)
  {
	  initlock(&bcache.hash_lock[i], "bcache");
  }

  memset(bcache.hash, 0, BUCKET_SIZE);

  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = 0;
    b->prev = 0;
    initsleeplock(&b->lock, "buffer");
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
  uint bucket = blockno % BUCKET_SIZE;
  b = bcache.hash[bucket];
  acquire(&bcache.hash_lock[bucket]);

  if (b)
  {
      struct buf* cur = b;
      do {
    	if (cur->dev == dev && cur->blockno == blockno)
    	{
    		cur->refcnt++;
    		release(&bcache.hash_lock[bucket]);
    		acquiresleep(&cur->lock);
    		return cur;
    	}
    	cur = cur->next;
      } while (cur != b);
  }
  release(&bcache.hash_lock[bucket]);

  acquire(&bcache.lock);
  acquire(&bcache.hash_lock[bucket]);
  b = bcache.hash[bucket];
  if (b)
  {
      struct buf* cur = b;
      do {
    	if (cur->dev == dev && cur->blockno == blockno)
    	{
    		cur->refcnt++;
    		release(&bcache.hash_lock[bucket]);
    		release(&bcache.lock);
    		acquiresleep(&cur->lock);
    		return cur;
    	}
    	cur = cur->next;
      } while (cur != b);
  }

	for (b = bcache.buf; b < bcache.buf+NBUF; b++)
	{
		if(b->refcnt == 0) 
		{
		  uint cur_blockno = b->blockno;
		  if (bcache.hash[cur_blockno])
		  {

		  }

		  b->dev = dev;
		  b->blockno = blockno;
		  b->valid = 0;
		  b->refcnt = 1;

		  if (bcache.hash[bucket])
		  {
			  struct buf* head = bcache.hash[bucket];
			  struct buf* tail = head->prev;
			  head->prev = b;
			  tail->next = b;
			  b->prev = tail;
			  b->next = head;
			  bcache.hash[bucket] = b;
		  }
		  else
		  {
			  bcache.hash[bucket] = b;
			  b->prev = b;
			  b->next = b;
		  }
		  release(&bcache.hash_lock[bucket]);
		  release(&bcache.lock);
		  acquiresleep(&b->lock);
		  return b;
		}
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

  releasesleep(&b->lock);

  uint bucket = b->blockno % BUCKET_SIZE;
  acquire(&bcache.hash_lock[bucket]);

  b->refcnt--;
  if (b->refcnt == 0)
  {
	uint blockno = b->blockno;	
	uint bucket = blockno % BUCKET_SIZE;

	if (bcache.hash[bucket] == b)
	{
		if (b == b->next)
		{
			bcache.hash[bucket] = 0;
		}
		else
		{
			b->prev->next = b->next;
			b->next->prev = b->prev;
			bcache.hash[bucket] = b->next;
		}
	}
	else
	{
		b->prev->next = b->next;
		b->next->prev = b->prev;
	}
	b->next = 0;
	b->prev = 0;
  }

  release(&bcache.hash_lock[bucket]);
}

void
bpin(struct buf *b) {
  uint bucket = b->blockno % BUCKET_SIZE;
  acquire(&bcache.hash_lock[bucket]);
  b->refcnt++;
  release(&bcache.hash_lock[bucket]);
  //acquire(&bcache.lock);
  //b->refcnt++;
  //release(&bcache.lock);
}

void
bunpin(struct buf *b) {
  uint bucket = b->blockno % BUCKET_SIZE;
  acquire(&bcache.hash_lock[bucket]);
  b->refcnt--;
  release(&bcache.hash_lock[bucket]);
  //acquire(&bcache.lock);
  //b->refcnt--;
  //release(&bcache.lock);
}


