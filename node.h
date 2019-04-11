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

/**
 * Introduce the "bool" type. Though I really love C/C++, I would still say
 *   it is stupid for not even defining a standard "bool" type in ANSI C
 *   standard.
 */
#define true  (1)
#define false (0)
typedef int bool;


/**
 * Structure of a block cell node.
 */
typedef struct node_t {
    int x;          /* X coordinate, starting from 0. */
    int y;          /* Y coordinate, starting from 0. */
    int gs;         /* A* g-score. */
    int fs;         /* A* f-score. */
    int heap_id;    /* Position on min heap, used by updating. */
    bool closed;    /* Has been closed? */
    struct node_t *parent;  /* Parent node along the path. */
} node_t;

/* Function prototypes. */
node_t *node_init(node_t *node, int x, int y);

#define node_less(n1, n2)   ((n1)->fs < (n2)->fs)

void init_pool();

node_t *alloc_node();

void release_pool();

#endif
