/**
 * File: node.c
 *
 *   Implementation of library functions manipulating a block cell node. Feel
 *     free to add, remove, or modify these library functions to serve your
 *     algorithm.
 *
 * Jose @ ShanghaiTech University
 */

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <stdlib.h>     /* NULL, malloc, free */
#include <limits.h>     /* INT_MAX */
#include <sys/mman.h>
#include <assert.h>
#include "node.h"

/**
 * Initialize a node whose position is recorded at (X, Y) with type MARK.
 *   Returns the pointer to the new node.
 */
node_t *node_init(node_t *node, int x, int y) {
    node->parent = NULL;
    node->x = x;
    node->y = y;
    node->gs = INT_MAX;
    node->fs = INT_MAX;
    node->heap_id = 0;
    return node;
}

void mem_pool_init(mem_pool_t *pool) {
    pool->start_chunk = mmap(
            NULL,
            NODE_MEM_MAP_SIZE,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS,
            -1, 0);
    assert(pool->start_chunk != MAP_FAILED);
    pool->end_chunk = pool->start_chunk;
    pool->end_chunk_len = (node_t *) pool->end_chunk + 1;
    pool->end_chunk_cap = (node_t *) ((size_t) pool->end_chunk + NODE_MEM_MAP_SIZE) - 1;
    *(void **) pool = NULL;
}

node_t *alloc_node(mem_pool_t *mem_pool) {
    if (mem_pool->end_chunk_len > mem_pool->end_chunk_cap) {
        void *new_chunk = mmap(
                (void *) ((size_t) mem_pool->end_chunk + NODE_MEM_MAP_SIZE),
                NODE_MEM_MAP_SIZE,
                PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS,
                -1, 0);
        assert(new_chunk != MAP_FAILED);
        *(void **) mem_pool->end_chunk = new_chunk;
        mem_pool->end_chunk = new_chunk;
        mem_pool->end_chunk_len = (node_t *) new_chunk + 1;
        mem_pool->end_chunk_cap = (node_t *) ((size_t) new_chunk + NODE_MEM_MAP_SIZE) - 1;
        *(void **) new_chunk = NULL;
    }

    return mem_pool->end_chunk_len++;
}

void mem_pool_destroy(mem_pool_t *mem_pool) {
    void *this_pool = mem_pool->start_chunk;
    while (this_pool != NULL) {
        void *next_pool = *(void **) this_pool;
        munmap(this_pool, NODE_MEM_MAP_SIZE);
        this_pool = next_pool;
    }
}
