#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

// lab10
#include "fcntl.h"
extern struct vma vrec[16];

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

  if (n+addr >= MAXVA)
  {
    return -1;
  }
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

// lab10
uint64
sys_mmap(void)
{
  uint64 addr;
  if(argaddr(0, &addr) < 0)
    return -1;

  int len;
  if (argint(1, &len) < 0)
	return -1;
	
  int prot;
  if (argint(2, &prot) < 0)
	  return -1;

  int flags;
  if (argint(3, &flags) < 0)
	  return -1;

  int fd;
  if (argint(4, &fd) < 0) 
	  return -1;
  
  int offset;
  if (argint(5, &offset) < 0)
	  return -1;

  struct proc *p = myproc();
  struct vma *pvma = 0;
  int i = 0;
  for (; i < 16; ++i){
	  if (vrec[i].pid == 0)
	  {
		  pvma = &vrec[i];
		  break;
	  }
  }
  if (pvma == 0)	
	  return -1;
  struct file* fp = p->ofile[fd];

  if (flags & MAP_SHARED)
  {
	  if (!checkread(fp) && (prot & PROT_READ))
		  return -1;

	  if (!checkwrite(fp) && (prot & PROT_WRITE))
		  return -1;
  }

  pvma->fp = fp;
  pvma->len = len;
  pvma->prot = prot;
  pvma->flags = flags;
  pvma->fd = fd;
  pvma->offset = offset;
  pvma->pid = p->pid;

  int pages = len/PGSIZE + ((len % PGSIZE) != 0);
  pvma->pages = pages;
  pvma->addr = p->start_address - pages * PGSIZE;
  addr = pvma->addr;

  p->start_address = pvma->addr;
  p->v[i] = 1;
  filedup(pvma->fp);

  return addr;
}

uint64
sys_munmap(void)
{
  uint64 addr;
  if(argaddr(0, &addr) < 0)
    return -1;

  int len;
  if (argint(1, &len) < 0)
	return -1;
  
  struct proc* p = myproc();
  struct vma* t = 0;

  int i = 0;
  for (; i < 16; ++i)
  {
	if (p->v[i] == 0)
		continue;
	t = &vrec[i];
	if (t->addr >= addr && addr < t->addr + PGSIZE * (t->pages))
		break;
  }
  if (t == 0)
	  return -1;

  int full = 0;
  int pages = len/PGSIZE + ((len % PGSIZE) != 0);

  if (t->addr != addr)
  {
	  printf("start address wrong\n");
	  return -1;
  }
  if ((t->addr == addr) && (pages == t->pages))
	  full = 1;
  if (t->flags & MAP_SHARED)
  {
	filewrite(t->fp, addr, pages*PGSIZE);
  }
  for (int i = 0; i < pages; ++i)
  {
	  uint64 cur_addr = addr + i * PGSIZE;
	  if (walkaddr(p->pagetable, cur_addr) != 0)
		  uvmunmap(p->pagetable, cur_addr, 1, 1);
  }
  if (full == 1)
  {
	  fileclose(t->fp);
	  p->v[i] = 0;
	  t->pid = 0;
  }
  else
  {
	t->addr += pages * PGSIZE;
	t->pages -= pages;
  }
  return 0;
}
