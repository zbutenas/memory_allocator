#include <stdlib.h>
#include <sys/mman.h>
#include <stdio.h>

#include "hmalloc.h"
#include "free_list.h"

/*
  typedef struct hm_stats {
  long pages_mapped;
  long pages_unmapped;
  long chunks_allocated;
  long chunks_freed;
  long free_length;
  } hm_stats;
*/

typedef struct size_mem {
    size_t size;
} size_mem;

free_list* freelist = NULL;
const size_t PAGE_SIZE = 4096;
static hm_stats stats; // This initializes the stats to 0.

void
coel()
{
    free_list* cpy = freelist;
    free_list* cp = freelist;
    for (; cpy; cpy = cpy->next){
	for (; cp; cp = cp->next) {
            if (&cpy + cpy->size == &cp) {
		void* add = &cpy;
		((free_list*) add)->size = cpy->size + cp->size;
		remove_free_list(cpy);
		remove_free_list(cp);
		add_free_list((free_list*) add);
	    }
	}
    }
}

long
free_list_length()
{
    long size = 0;
    free_list* cpy = freelist;
    for (; cpy; cpy->next) {
	if (cpy != NULL) {
            size += 1;
	}
    }
    return size;
}
	
void
add_free_list(free_list* add)
{
    free_list* cpy = freelist;
    for (; cpy; cpy = cpy->next){

	// Checks to see where the added item fits in
	if (cpy != NULL && cpy->next != NULL &&
		cpy->size < add->size && cpy->next->size
		> add->size) {
	    add->next = cpy->next;
	    cpy->next = add;
	}

        // If the current free list has a length of zero
	else if (free_list_length() == 0){
	    cpy = add;
	}

	// If the current list has a length of one
	else if (free_list_length() == 1){
	    if (cpy->size < add->size) {
		cpy->next = add;
	    }
	    else {
		add->next = cpy;
		cpy = add;
	    }
	}
    }

    coel();
} 

void 
remove_free_list(free_list* removed)
{
    free_list* cpy = freelist;
    for (; cpy; cpy = cpy->next) {

	// If not
	if (cpy->next != NULL && cpy->next == removed) {
	    cpy->next = cpy->next->next;
 	}


	// If the length of the list is one
	else if (free_list_length() == 1) {
	    if (cpy = removed) {
		cpy = NULL;
		break;
	    }
	}
    }
}

hm_stats*
hgetstats()
{
    stats.free_length = free_list_length();
    return &stats;
}

void
hprintstats()
{
    stats.free_length = free_list_length();
    fprintf(stderr, "\n== husky malloc stats ==\n");
    fprintf(stderr, "Mapped:   %ld\n", stats.pages_mapped);
    fprintf(stderr, "Unmapped: %ld\n", stats.pages_unmapped);
    fprintf(stderr, "Allocs:   %ld\n", stats.chunks_allocated);
    fprintf(stderr, "Frees:    %ld\n", stats.chunks_freed);
    fprintf(stderr, "Freelen:  %ld\n", stats.free_length);
}

static
size_t
div_up(size_t xx, size_t yy)
{
    // This is useful to calculate # of pages
    // for large allocations.
    size_t zz = xx / yy;

    if (zz * yy == xx) {
        return zz;
    }
    else {
        return zz + 1;
    }
}

void*
hmalloc(size_t size)
{
    stats.chunks_allocated += 1;
    size += sizeof(size_t);

    // For extra big pages
    if (size >= PAGE_SIZE) {
	int np = div_up(size, PAGE_SIZE);
	stats.pages_mapped += np;
	void* m = mmap(0, np * PAGE_SIZE, PROT_READ|PROT_WRITE,
		       	MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	((size_mem*) m)->size  = np * PAGE_SIZE;
        return (m + 8);
    }

    // For smaller pages
    else {
	free_list* cpy = freelist;

	// If we have memory available in the free list
	for (; cpy; cpy = cpy->next) {
	    if (cpy->size >= size) {
		size_t sf = cpy->size;
		remove_free_list(cpy);
		void* f = &cpy + size;
	        ((free_list*) f)->size = sf - size;
                add_free_list(f);
                return (cpy + 8);
	    }
	}

	// If no memory is available in the free list
	stats.pages_mapped += 1;
	void* m = mmap(0, PAGE_SIZE, PROT_READ|PROT_WRITE,
		       	MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	((size_mem*) m)->size = PAGE_SIZE;
        return (m + 8);
    }
}

void
hfree(void* item1)
{
    stats.chunks_freed += 1;
    void* item = ((char*) item1) - 8;
    
    if (((size_mem*) item)->size  > PAGE_SIZE) {
	 int np = div_up(((size_mem*) item)->size, PAGE_SIZE);
	 stats.pages_unmapped += np;
	 munmap(item, (((size_mem*) item)->size));
    }
    else {
        add_free_list((free_list*) item);
    }
}


