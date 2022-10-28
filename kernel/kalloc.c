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

#define INDEX(pa) (((uint64)pa-PGROUNDUP((uint64)KERNBASE))/PGSIZE)

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

struct{
  struct spinlock lock;
  uint8 page_ref[(PHYSTOP - PGROUNDUP(KERNBASE)) / PGSIZE];
} kref;

void
kinit()
{
  initlock(&kref.lock, "kref");
  acquire(&kref.lock);
  // set all pages` ref count to 1 so that kfree can work properly
  // memset(kref.page_ref, 1, INDEX(PHYSTOP));
  for(int i = 0; i < INDEX(PHYSTOP); kref.page_ref[i++] = 1);
  release(&kref.lock);
  initlock(&kmem.lock, "kmem");
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
  char ref;
  acquire(&kref.lock);
  if((ref = kref.page_ref[INDEX(pa)]--) == 0){
      kref.page_ref[INDEX(pa)] = 0;
      panic("kfree: double free");
  }
  release(&kref.lock);
  if(ref == 1){  
    memset(pa, 1, PGSIZE);
    // update page ref count and free the physical page when no ref exits
    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    // Fill with junk to catch dangling refs.
    release(&kmem.lock);
  }
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

  

  if(r){
    memset((char*)r, 5, PGSIZE); // fill with junk
    acquire(&kref.lock);
    kref.page_ref[INDEX(r)] = 1; // set page ref to 1
    release(&kref.lock);
  }
  return (void*)r;
}

int kaddref(uint64 pa, char change){ // change: 0 return current ref count; else increment ref count by 1
  if(change != 0){
    acquire(&kref.lock);
    if(++kref.page_ref[INDEX(pa)] == 1){
      release(&kref.lock);
      panic("kref: trying to refer to an unalloced page!");
    }
    release(&kref.lock);
  }

  return kref.page_ref[INDEX(pa)];
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