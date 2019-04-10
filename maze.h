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
    node_t start;      /* Start node pointer. */
    node_t goal;       /* Goal node pointer. */
    node_t wall;
    int fd;
    void *mem_map;
    size_t mem_size;
    char **lines;
} maze_t;

/* Function prototypes. */
void maze_init(maze_t *maze, char *filename);

void maze_destroy(maze_t *maze);

node_t *maze_set_cell(maze_t *maze, int x, int y);

node_t *maze_get_cell(maze_t *maze, int x, int y);

char *get_char_loc(maze_t *maze, int x, int y);

#endif
