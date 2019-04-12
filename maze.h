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

#define maze_node(maze, x, y)       ((maze)->nodes[(y) * (maze)->cols + (x)])
#define maze_lines(file, x, y)      ((file)->lines[y][x])
#define get_closed(maze)             (&((maze)->closed))
#define get_goal(maze)              (&((maze)->goal))
#define get_wall(maze)              (&((maze)->wall))
#define is_closed(maze, ptr)         ((ptr) == &((maze)->closed))
#define is_goal(maze, ptr)          ((ptr) == &((maze)->goal))
#define is_wall(maze, ptr)          ((ptr) == &((maze)->wall))


typedef struct maze_file_t {
    int rows;               /* Number of rows. */
    int cols;               /* Number of cols. */
    int fd;                 /* file descriptor. */
    void *mem_map;          /* memory map. */
    size_t mem_size;        /* memory map size. */
    char **lines;           /* lines pointer. */
} maze_file_t;

/**
 * Structure of a minecraft-style block maze.
 */
typedef struct maze_t {
    node_t **nodes;         /* Array of node. */
    node_t closed;            /* Goal node. */
    node_t goal;            /* Goal node. */
    node_t wall;            /* Wall node. */
    int cols;               /* Number of cols. */
} maze_t;

/* Function prototypes. */
maze_t *maze_init(int rows, int cols);

void maze_destroy(maze_t *maze);

maze_file_t *maze_file_init(char *filename);

void maze_file_destroy(maze_file_t *file);

#endif
