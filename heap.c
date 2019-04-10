/**
 * File: heap.c
 * 
 *   Implementation of library functions manipulating a min priority queue.
 *     Feel free to add, remove, or modify these library functions to serve
 *     your algorithm.
 *  
 * Jose @ ShanghaiTech University
 */

#include <stdlib.h>     /* malloc, free */
#include <limits.h>     /* INT_MAX */
#include "heap.h"

/**
 * Initialize a min heap. The heap is constructed using array-based
 *   implementation. Returns the pointer to the new heap.
 */
void heap_init(heap_t *heap) {
    heap->size = 0;
    heap->capacity = INIT_CAPACITY;    /* Initial capacity = 1000. */
    heap->nodes = malloc(INIT_CAPACITY * sizeof(node_t *));
    heap->nodes[0] = malloc(sizeof(node_t));
    heap->nodes[0]->fs = -INT_MAX;      /* Dummy node at index 0. */
}

/**
 * Delete the memory occupied by the min heap H.
 */
void heap_destroy(heap_t *heap) {
    free(heap->nodes[0]);
    free(heap->nodes);
}

/**
 * Insert a node N into the min heap H.
 */
void heap_insert(heap_t *heap, node_t *node) {
    int cur = ++heap->size;    /* Index 0 lays dummy node, so increment first. */
    heap->nodes[heap->size] = node;
    while (node_less(node, heap->nodes[cur / 2])) {
        heap->nodes[cur] = heap->nodes[cur / 2];
        heap->nodes[cur]->heap_id = cur;
        cur /= 2;
    }
    heap->nodes[cur] = node;
    node->heap_id = cur;

    /* If will exceed current capacity, doubles the capacity. */
    if (heap->size == heap->capacity) {
        heap->capacity *= 2;
        heap->nodes = realloc(heap->nodes, heap->capacity * sizeof(node_t *));
    }
}

/**
 * Extract the root (i.e. the minimum node) in min heap H. Returns the pointer
 *   to the extracted node.
 */
node_t *heap_extract(heap_t *heap) {
    node_t *ret = heap->nodes[1];
    node_t *last = heap->nodes[heap->size--];
    int cur, child;
    for (cur = 1; 2 * cur <= heap->size; cur = child) {
        child = 2 * cur;
        if (child < heap->size && node_less(heap->nodes[child + 1], heap->nodes[child]))
            child++;
        if (node_less(heap->nodes[child], last)) {
            heap->nodes[cur] = heap->nodes[child];
            heap->nodes[cur]->heap_id = cur;
        } else
            break;
    }
    heap->nodes[cur] = last;
    last->heap_id = cur;
    return ret;
}

/**
 * Update the min heap H in case that node N has changed its f-score.
 */
void heap_update(heap_t *heap, node_t *node) {
    int cur = node->heap_id;
    while (node_less(node, heap->nodes[cur / 2])) {
        heap->nodes[cur] = heap->nodes[cur / 2];
        heap->nodes[cur]->heap_id = cur;
        cur /= 2;
    }
    heap->nodes[cur] = node;
    node->heap_id = cur;
}


