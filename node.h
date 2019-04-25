/**
 * File: node.h
 * 
 *   Declaration of a block cell node data structure and library functions
 *     manipulating the node. Feel free to add, remove, or modify these
 *     declarations to serve your algorithm.
 *  
 * Jose @ ShanghaiTech University
 */

#ifndef _NODE_H_
#define _NODE_H_

#define node_less(n1, n2)   ((n1)->fs < (n2)->fs)
#define NODE_MEM_MAP_SIZE        (0x10000)


/**
 * Structure of a block cell node.
 */
typedef struct node_t {
    struct node_t *parent;      /* Parent node along the path. */
    int x;                      /* X coordinate, starting from 0. */
    int y;                      /* Y coordinate, starting from 0. */
    int gs;                     /* A* g-score. */
    int fs;                     /* A* f-score. */
    int heap_id;                /* Position on min heap, used by updating. */
} node_t;

typedef struct mem_pool_t {
    void *start_chunk;
    void *end_chunk;
    node_t *end_chunk_cap;
    node_t *end_chunk_len;
} mem_pool_t;

/* Function prototypes. */
node_t *node_init(node_t *node, int x, int y);

void mem_pool_init(mem_pool_t *pool);

node_t *alloc_node(mem_pool_t *mem_pool);

void mem_pool_destroy(mem_pool_t *mem_pool);

#endif
