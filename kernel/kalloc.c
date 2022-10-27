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

#define INDEX(pa) (((uint64)pa-PGROUNDUP((uint64)end))/PGSIZE)

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  uint8 page_ref[(PHYSTOP - KERNBASE) / PGSIZE];
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  acquire(&kmem.lock);
  // set all pages` ref count to 1 so that kfree can work properly
  memset(kmem.page_ref, 1, INDEX(PHYSTOP));
  release(&kmem.lock);
  // for(int i = (PHYSTOP - KERNBASE) / PGSIZE; i--; kmem.page_ref[i] = 1);
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

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  r = (struct run*)pa;
  acquire(&kmem.lock);
  // update page ref count and free the physical page when no ref exits
  switch(kmem.page_ref[INDEX(pa)]){
    case 0:
      panic("kfree: double free");
    case 1:
      r->next = kmem.freelist;
      kmem.freelist = r;
      // Fill with junk to catch dangling refs.
      memset(pa, 1, PGSIZE);
    default:
      --kmem.page_ref[INDEX(pa)];
      break;
  }
  release(&kmem.lock);
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
    kmem.page_ref[INDEX(r)] = 1; // set page ref to 1
  }
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

int kref(uint64 pa, char change){ // change: 0 return current ref count; else increment ref count by 1
  int index = (pa - KERNBASE) / PGSIZE;
  if(change != 0){
    acquire(&kmem.lock);
    if(++kmem.page_ref[index] == 1)
      panic("kref: trying to refer to an unalloced page!");
    release(&kmem.lock);
  }

  return kmem.page_ref[index];
}

// alloc a new page(writable) and copy the content into it
// deref the origional physical page
// return the newly alloced physical page
char * cow(pagetable_t pagetable, uint64 va){
  char * mem = kalloc();
  if(mem == 0){
    printf("cow:failed to alloc required page\n");
    return 0;
  }
  pte_t * pte = walk(pagetable, va, 0);
  memmove(mem, (void*)PGROUNDDOWN(PTE2PA(*pte)), PGSIZE);
  kfree((void*)PTE2PA(*pte));
  *pte = PA2PTE(mem) | ((PTE_FLAGS(*pte) | PTE_W) & ~PTE_C);
  return mem; 
}