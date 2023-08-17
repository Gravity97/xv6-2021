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
} kmem;

struct {
  struct spinlock lock;
  int refcount;
} refcounts[(PHYSTOP - KERNBASE) / PGSIZE];

void
kinit() {
  for (int i = 0; i < (PHYSTOP - KERNBASE) / PGSIZE; i++){
    initlock(&refcounts[i].lock, "reflog");
  }
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    kfree(p);
    refcounts[PAINDEX((uint64)p)].refcount = 1;
  }
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

  acquirereflock(pa);
  refdecre(pa);
  if (refcounts[PAINDEX((uint64)pa)].refcount <= 0) {
    // Fill with junk to catch dangling refs.
    memset(pa, 1, PGSIZE);

    r = (struct run*)pa;

    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
  }
  releasereflock(pa);
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

    acquirereflock((void*)r);
    refcounts[PAINDEX((uint64)r)].refcount = 1;
    releasereflock((void*)r);
  }
  return (void*)r;
}

void 
refincre(void* pa)
{
  refcounts[PAINDEX((uint64)pa)].refcount++;
}

void 
refdecre(void* pa)
{
  refcounts[PAINDEX((uint64)pa)].refcount--;
}

void 
acquirereflock(void* pa)
{
  acquire(&refcounts[PAINDEX((uint64)pa)].lock);
}

void 
releasereflock(void* pa)
{
  release(&refcounts[PAINDEX((uint64)pa)].lock);
}

void*
cowkalloc(pagetable_t pagetable, uint64 pa)
{
  acquirereflock((void*)pa);
  if(refcounts[PAINDEX(pa)].refcount <= 1){
    // if <= 1, we need not to alloc a new page.
    releasereflock((void*)pa);
    return (void*)pa;
  }

  char* mem;
  if((mem = kalloc()) == 0){
    releasereflock((void*)pa);
    return 0;
  }
  memmove(mem, (void*)pa, PGSIZE);
  refdecre((void*)pa);
  releasereflock((void*)pa);
  return (void*)mem;
}
