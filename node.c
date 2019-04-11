/**
 * File: node.c
 * 
 *   Implementation of library functions manipulating a block cell node. Feel
 *     free to add, remove, or modify these library functions to serve your
 *     algorithm.
 *  
 * Jose @ ShanghaiTech University
 */

#include <stdlib.h>     /* NULL, malloc, free */
#include <limits.h>     /* INT_MAX */
#include <sys/mman.h>
#include <assert.h>
#include "node.h"


#define MEM_MAP_SIZE    (0x10000)

static const size_t POOL_SIZE = MEM_MAP_SIZE / sizeof(node_t) - 1;
static void *mem_pool = NULL;
static void *last_pool = NULL;
static size_t last_pool_length = 0;

/**
 * Initialize a node whose position is recorded at (X, Y) with type MARK.
 *   Returns the pointer to the new node.
 */
node_t *node_init(node_t *node, int x, int y) {
    node->x = x;
    node->y = y;
    node->gs = INT_MAX;
    node->fs = INT_MAX;
    node->heap_id = 0;
    node->opened = false;
    node->closed = false;
    node->parent = NULL;
    return node;
}

/**
 * Defines the comparison method between nodes, that is, comparing their
 *   A* f-scores.
 */
bool node_less(node_t *n1, node_t *n2) {
    return n1->fs < n2->fs;
}

void init_pool() {
    mem_pool = mmap(
            NULL,
            MEM_MAP_SIZE,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE| MAP_ANONYMOUS,
            -1, 0);
    assert(mem_pool != MAP_FAILED);
    last_pool = mem_pool;
    last_pool_length = 1;
    *(void **) mem_pool = NULL;
}


node_t *alloc_node() {
    if (last_pool_length >= POOL_SIZE) {
        void *new_block = mmap(
                last_pool,
                MEM_MAP_SIZE,
                PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS,
                -1, 0);
        assert(new_block != MAP_FAILED);
        *(void **) last_pool = new_block;
        last_pool = new_block;
        last_pool_length = 1;
        *(void **) new_block = NULL;
    }

    return (node_t *) last_pool + last_pool_length++;
}

void release_pool() {
    void *this_pool = mem_pool;
    while(this_pool != NULL) {
        void *next_pool = *(void **)this_pool;
        munmap(this_pool, MEM_MAP_SIZE);
        this_pool = next_pool;
    }
    mem_pool = NULL;
    last_pool = NULL;
    last_pool_length = 0;
}
