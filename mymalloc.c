#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#include <malloc.h>
#include <stdio.h>

#include <unistd.h> // sbrk
#include <string.h> // memset
#include <assert.h> // assert
#include <stdint.h> // SIZE_MAX, INTPTR_MAX

// NOTE: You should NOT include <stdlib.h> in your final implementation.
// malloc.h turns every call to malloc/calloc/free in this file into a call
// to mymalloc/mycalloc/myfree, so calling them here would recurse forever.

#include <debug.h> // definition of debug_printf

/*
 * Header stored directly in front of every chunk of memory that is handed out.
 * All blocks (free and in use) are linked in the order that sbrk gave them.
 */
typedef struct block {
  size_t size; // bytes of user memory after this header
  struct block *next; // next block in the heap (free or not)
  int free; // 1 if this block can be reused, 0 if in use
} block_t;

#define BLOCK_SIZE sizeof(block_t)

// First and last blocks in our list. NULL until the first malloc.
static block_t *head = NULL;
static block_t *tail = NULL;

/*
 * Finds the first free block that can hold s bytes (first fit).
 * Parameters: s - number of bytes requested.
 * Returns: pointer to that block's header or NULL if none fits.
 */
static block_t *find_free_block(size_t s) {
  block_t *curr = head;
  while (curr != NULL) {
    if (curr->free && curr->size >= s) {
      return curr;
    }
    curr = curr->next;
  }
  return NULL;
}

/*
 * Grows the heap by exactly one header plus s bytes and adds the new block
 * to the end of the list. Assumes no existing free block was big enough.
 * Parameters: s - number of user bytes needed.
 * Returns: the new block's header, or NULL if sbrk fails.
 */
static block_t *request_block(size_t s) {
  if (s > (size_t) INTPTR_MAX - BLOCK_SIZE) {
    return NULL; // too big for sbrk's argument type
  }
  void *mem = sbrk(BLOCK_SIZE + s);
  if (mem == (void *) -1) {
    return NULL; // if the OS refused
  }

  block_t *b = (block_t *) mem; // header goes at the start
  b->size = s;
  b->next = NULL;
  b->free = 0;

  if (head == NULL) {
    head = b;
  } else {
    assert(tail != NULL);
    tail->next = b;
  }
  tail = b;
  return b;
}

/*
 * Allocates s bytes, reusing a free block when possible.
 * Parameters: s - number of bytes requested.
 * Returns: pointer to usable memory (just past the header) or NULL if s is
 * 0 or memory can't be obtained.
 */
void *mymalloc(size_t s) {
  if (s == 0) {
    return NULL; // malloc(0) prints nothing and returns NULL
  }
  debug_printf("Malloc %zu bytes\n", s);

  block_t *b = find_free_block(s);
  if (b != NULL) {
    b->free = 0; // reuse an old block
  } else {
    b = request_block(s);
    if (b == NULL) {
      return NULL; // out of memory
    }
  }
  assert(b->size >= s && b->free == 0);
  return (void *) (b + 1); // skip past the header
}

/*
 * Allocates an array of nmemb elements of s bytes each, all zeroed.
 * Parameters: nmemb - number of elements; s - size of each element.
 * Returns: pointer to zeroed memory or NULL on overflow or failure.
 */
void *mycalloc(size_t nmemb, size_t s) {
  if (nmemb != 0 && s > SIZE_MAX / nmemb) {
    return NULL; // nmemb * s would overflow
  }
  size_t total = nmemb * s;

  void *p = mymalloc(total); // prints "Malloc ..." first
  if (p == NULL) {
    return NULL; // out of memory
  }
  debug_printf("Calloc %zu bytes\n", total);
  memset(p, 0, total); // a reused block may still hold old data
  return p;
}

/*
 * Marks the block holding ptr as free so it can be reused.
 * Parameters: ptr - pointer previously returned by mymalloc/mycalloc.
 * Returns: nothing. Does nothing if ptr is NULL.
 */
void myfree(void *ptr) {
  if (ptr == NULL) {
    return; // free(NULL) does and prints nothing
  }
  block_t *b = (block_t *) ptr - 1; // go back to the header
  assert(b->free == 0); // catches double frees
  b->free = 1;
  debug_printf("Freed %zu bytes\n", b->size);
}
