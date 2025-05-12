/////////////////////////////////////////////////////////////////////////////////////////
// INTEGRITY STATEMENT (v4)
//
// By signing your name and ID below you are stating that you have agreed
// to the online academic integrity statement posted on edX:
// (Course > Assignments > Assignment Information & Policies > Academic Integrity Policy)
/////////////////////////////////////////////////////////////////////////////////////////
// I received help from and/or collaborated with:

// 0
//
// Name: Sahel Hossain
// login ID: s53hossa
//////////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include "contiguous.h"

struct contiguous {
  struct cnode *first;
  void *upper_limit;
};

struct cnode {
  size_t nsize;
  struct cnode *prev;
  struct cnode *next;
  struct contiguous *block;
};

const int SIZEOF_CONTIGUOUS = sizeof(struct contiguous);
const int SIZEOF_CNODE = sizeof(struct cnode);



static const char STAR_STR[] = "*";
static const char NULL_STR[] = "NULL";

// maybe_null(void *p) return a pointer to "NULL" or "*",
//   indicating if p is NULL or not.
static const char *maybe_null(void *p) {
  return p ? STAR_STR : NULL_STR;
}

// gapsize(n0, n1) determine the size (in bytes) of the gap between n0 and n1.
static size_t gapsize(struct cnode *n0, struct cnode *n1) {
  assert(n0);
  assert(n1);
  void *v0 = n0;
  void *v1 = n1;
  return (v1 - v0) - n0->nsize - sizeof(struct cnode);
}

// print_gapsize(n0, n1) print the size of the gap between n0 and n1,
//     if it's non-zero.
static void print_gapsize(struct cnode *n0, struct cnode *n1) {
  assert(n0);
  assert(n1);
  size_t gap = gapsize(n0, n1);
  
  if (gap != 0) { 
    printf("%zd byte gap\n", gap);
  }
}


// pretty_print_block(chs, size) Print size bytes, starting at chs,
//    in a human-readable format: printable characters other than backslash
//    are printed directly; other characters are escaped as \xXX
static void pretty_print_block(unsigned char *chs, int size) {
  assert(chs);
  for (int i = 0; i < size; i++) {
    printf(0x20 <= chs[i] && chs[i] < 0x80 && chs[i] != '\\'
           ? "%c" : "\\x%02X", chs[i]);
  }
  printf("\n");
}

// print_node(node) Print the contents of node and all nodes that
//    follow it.  Return a pointer to the last node.
static struct cnode *print_node(struct cnode *node) {
  while (node != NULL) {
    void *raw = node + 1;     // point at raw data that follows.
    printf("struct cnode\n");
    printf("    nsize: %ld\n", node->nsize);
    printf("    prev: %s\n", maybe_null(node->prev));
    printf("    next: %s\n",  maybe_null(node->next));

    printf("%zd byte chunk: ", node->nsize);
    
    pretty_print_block(raw, node->nsize);
    
    if (node->next == NULL) {
      return node;
    } else {
      print_gapsize(node, node->next);
      node = node->next;
    }
  }
  return NULL;
}



static void print_hr(void) {
    printf("----------------------------------------------------------------\n");
}

// print_debug(block) print a long message showing the content of block.
void print_debug(struct contiguous *block) {
  assert(block);
  void *raw = block;

  print_hr();
  printf("struct contiguous\n");
  printf("    first: %s\n", maybe_null(block->first));

  if (block->first == NULL) {
    size_t gap = block->upper_limit - raw - sizeof(struct contiguous);
    printf("%zd byte gap\n", gap);           
  } else {
    void *block_first = block->first;
    size_t gap = block_first - raw - sizeof(struct contiguous);
    if (gap) {
      printf("%zd byte gap\n", gap);
    }
  }
 
  struct cnode *lastnode = print_node(block->first);
  
  if (lastnode != NULL) {
    print_gapsize(lastnode, block->upper_limit);
  }

  print_hr();
}


// see contiguous.h for documentation
struct contiguous *make_contiguous(size_t size) {
  assert(size > sizeof(struct contiguous));

  // initializes a contiguous struct c
  void *ptr = malloc(size);
  struct contiguous *c = ptr;
  char *bytes = ptr;
  c->first = NULL;
  c->upper_limit = bytes + size;

  // fills the rest with '$' characters
  for (size_t i = sizeof(struct contiguous); i < size; i++) {
    *(bytes + i) = '$';
  }
  return c;
}

// see contiguous.h for documentation
void destroy_contiguous(struct contiguous *block) {
  assert(block);
  if (block->first != NULL) {
    printf("Destroying non-empty block!\n");
  }
  free(block);
}

// see contiguous.h for documentation
void cfree(void *p) {
  // case 0: error protection
  if (p == NULL) {
    return;
  }

  struct cnode *c = p;
  c -= 1;
  // case 1: cnode is linked both ways to another cnode
  if (c->prev != NULL && c->next != NULL) {
    c->prev->next = c->next;
    c->next->prev = c->prev;
  }
  // case 2: cnode is after contiguous and is linked to another cnode
  else if (c->prev == NULL && c->next != NULL) {
    c->next->prev = NULL;
    c->block->first = c->next;
  }
  // case 3: very last cnode
  else if (c->prev != NULL && c->next == NULL) {
    c->prev->next = NULL;
  }
  // case 4: only one cnode after contiguous
  else {
    c->block->first = NULL;
  }
}


// make_cnode(c, prev, next, block, nsize) initializes c
// Time: O(1);
void make_cnode(struct cnode *c, struct cnode *prev, struct cnode *next, 
                struct contiguous *block, int nsize) {
assert(c && block);
assert(nsize >= 0);
c->prev = prev;
c->next = next;
c->block = block;
c->nsize = nsize;
}


// see contiguous.h for documentation
// requires: block is valid
//           size is non-negative
// Time: O(n), where n is the number of linked nodes
void *cmalloc(struct contiguous *block, int size) {
  assert(block);
  assert(size >= 0);

  size_t gap_needed = size + sizeof(struct cnode);
  void *mem_end = block->upper_limit;   // pointer to VERY FIRST UNALLOCATED memory
  void *mem_start = block + 1;          // memory that we can SAFELY ASSUME is UNUSED

  // case 1: no cnodes; inserting between contiguous and end of mem
  if (block->first == NULL) {
    if (mem_end - mem_start < gap_needed) {
      return NULL;
    }
    else {                                  // places a cnode right next to the contiguous
      struct cnode *new_c = mem_start;
      make_cnode(new_c, NULL, NULL, block, size);
      block->first = new_c;

      void *first_chunk = new_c + 1;
      return first_chunk;                   // address of associated chunk
    }
  }


  // case 2: the gap between the block and first cnode is sufficient
  if (block->first != NULL) {
    void *first_cnode = block->first;
    size_t gap = mem_start - first_cnode;

    if (gap >= gap_needed) {
      struct cnode *new_c = mem_start;
      make_cnode(new_c, NULL, block->first, block, size);
      
      // relinks list if there are other cnodes
      block->first->prev = new_c;
      block->first = new_c;
    
      void *first_chunk = new_c + 1;
      return first_chunk; 
    }
  }

  // case 3: first gap wasnt sufficient, then check space between cnodes
  struct cnode *c1 = block->first;
  struct cnode *c2 = c1->next;
  
  // case 3.a: there is only one cnode, checking gap between first cnode and end of mem
  if (c2 == NULL) {
    c2 = mem_end;
  }
  // case 3.b: there is are cnodes, find closest gap if exists
  else {
    // while loop either stops at a sufficient gap or when there is no next cnode
    while (gapsize(c1, c2) < gap_needed) {
      c1 = c2;
      if (c2->next == NULL) {
        c2 = mem_end;
        break;
      }
      c2 = c2->next;
    }
  }
  
  if (gapsize(c1, c2) >= gap_needed) {
    mem_start = c1 + 1;
    mem_start += c1->nsize;
    struct cnode *new_c = mem_start;
    
    // insert A: inserting between a cnode and end of mem
    if (c2 == mem_end) {
      make_cnode(new_c, c1, NULL, block, size);
      c1->next = new_c;

      void *first_chunk = new_c + 1;
      return first_chunk;
    }
    // insert B: inserting between 2 nodes
    else {
      make_cnode(new_c, c1, c2, block, size);
      c1->next = new_c;
      c2->prev = new_c;

      void *first_chunk = new_c + 1;
      return first_chunk;
    }

  }
  // case 4: no gap found
  return NULL;
}




  




