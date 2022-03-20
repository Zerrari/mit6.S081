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

struct {
  struct spinlock lock;
  struct run *freelist;

  // lab6
  struct spinlock page_lock;
  int page_refer[(uint64)PHYSTOP/PGSIZE];
} kmem;

void
inc_refer(uint64 pa)
{
  acquire(&kmem.lock);
  kmem.page_refer[pa/PGSIZE]++;
  release(&kmem.lock);
}

void 
dec_refer(uint64 pa)
{
  acquire(&kmem.lock);
  kmem.page_refer[pa/PGSIZE]--;
  release(&kmem.lock);
}

int 
get_refer(uint64 pa)
{
	return kmem.page_refer[pa/PGSIZE];
}

void
kinit()
{
  initlock(&kmem.lock, "kmem");

  // lab6
  memset(kmem.page_refer, 0, (uint64)PHYSTOP/PGSIZE);

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

  acquire(&kmem.page_lock);
  if (kmem.page_refer[(uint64)pa/PGSIZE] == 0 || --kmem.page_refer[(uint64)pa/PGSIZE] == 0)
  {
	  // Fill with junk to catch dangling refs.
	  memset(pa, 1, PGSIZE);

	  r = (struct run*)pa;

	  acquire(&kmem.lock);
	  r->next = kmem.freelist;
	  kmem.freelist = r;
	  release(&kmem.lock);
  }
  
  release(&kmem.page_lock);

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
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
	kmem.page_refer[(uint64)r/PGSIZE] = 1;

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
