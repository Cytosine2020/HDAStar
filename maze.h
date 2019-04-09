/**
 * File: maze.h
 * 
 *   Declaration of a minecraft-style block maze data structure and library
 *     functions manipulating the maze. Feel free to add, remove, or modify
 *     these declarations to serve your algorithm.
 *  
 * Jose @ ShanghaiTech University
 *
 */

#ifndef _MAZE_H_
#define _MAZE_H_

#include <stdio.h>  /* FILE */
#include "node.h"

/**
 * Structure of a minecraft-style block maze.
 */
typedef struct maze_t {
    int rows;           /* Number of rows. */
    int cols;           /* Number of cols. */
    node_t **nodes;     /* Array of node pointers. */
    node_t *start;      /* Start node pointer. */
    node_t *goal;       /* Goal node pointer. */
    FILE *file;         /* Source file pointer. */
} maze_t;

/* Function prototypes. */
maze_t *maze_init(char *filename);

void maze_destroy(maze_t *maze);

void maze_set_cell(maze_t *maze, int x, int y, mark_t mark);

node_t *maze_get_cell(maze_t *maze, int x, int y);

void maze_print_step(maze_t *maze, node_t *node);

#endif
