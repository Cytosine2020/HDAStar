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
#include "node.h"

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

node_t *alloc_node() {
    return NULL;
}

void release_pool() {

}
