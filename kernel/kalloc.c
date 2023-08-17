// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

#define MAXST 64

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  uint64 wl[MAXST];        //waiting list
} kmem[NCPU];

void
kinit()
{
  for (int i = 0; i < NCPU; i++){
    char lockname[16] = {0};
    snprintf(lockname, 16, "kmemlock%d", i + 1);
    initlock(&kmem[i].lock, lockname);
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

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  push_off();  //shutdown interrupt
  int id = cpuid();
  pop_off();   //restart interrupt

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

  push_off();  //shutdown interrupt
  int id = cpuid();
  
  acquire(&kmem[id].lock);
  r = kmem[id].freelist;
  if(r){
    kmem[id].freelist = r->next;
    release(&kmem[id].lock);
  }
  else{
    // no free mem, need to steal.
    release(&kmem[id].lock);

    int count = steal(id);
    if(count > 0){
      acquire(&kmem[id].lock);
      wlalloc(id, count);
      r = kmem[id].freelist;
      kmem[id].freelist = r->next;
      release(&kmem[id].lock);
    }
  }

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk

  pop_off();   //restart interrupt
  return (void*)r;
}

int 
steal(int id)
{
  memset(kmem[id].wl, 0, MAXST);

  int count = 0;
  for (int i = 0; i < NCPU; i++){
    if(i == id)
      continue;

    acquire(&kmem[i].lock);
    while(kmem[i].freelist && (MAXST - count) > 0){
      kmem[id].wl[count++] = (uint64)kmem[i].freelist;
      kmem[i].freelist = kmem[i].freelist->next;
    }
    release(&kmem[i].lock);

    if(count == MAXST)
      break;
  }

  return count;
}

void 
wlalloc(int id, int sz)
{
  for (int i = 0; i < sz && i < MAXST; i++){
    if(!kmem[id].wl[i])
      break;

    struct run* p = (struct run*)kmem[id].wl[i];
    (p)->next = kmem[id].freelist;
    kmem[id].freelist = p;
  }
}
