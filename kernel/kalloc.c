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

//struct {
//  struct spinlock lock;
//  struct run *freelist;
//} kmem;

// lab8
struct {
  struct spinlock lock;
  struct run *freelist;
} kmem[3];

void startfree(void *pa, int i);

void
kinit()
{
  //initlock(&kmem.lock, "kmem");
  initlock(&kmem[0].lock, "kmem0");
  initlock(&kmem[1].lock, "kmem1");
  initlock(&kmem[2].lock, "kmem2");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);

  int i = 0;

  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    //kfree(p);
	startfree(p, i++);
}


// lab8
void
startfree(void *pa, int i)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  int cpuid = i % 3;

  acquire(&kmem[cpuid].lock);
  r->next = kmem[cpuid].freelist;
  kmem[cpuid].freelist = r;
  release(&kmem[cpuid].lock);
	
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

  push_off();

  int id = cpuid();

  pop_off();

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem[id].lock);
  r->next = kmem[id].freelist;
  kmem[id].freelist = r;
  release(&kmem[id].lock);
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

  pop_off();

  int flag = 0;
  int prev = id;

  while (1)
  {
	  acquire(&kmem[id].lock);
	  r = kmem[id].freelist;
	  if(r)
	  {
		flag = 1;
		kmem[id].freelist = r->next;

	  }
	  release(&kmem[id].lock);
	  if (flag)
		  break;
	  id = (id + 1) % 3;
	  if (prev == id)
		  break;
  }


  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
