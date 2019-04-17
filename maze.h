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
#define get_goal(maze)              (&((maze)->goal))


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
    node_t goal;            /* Goal node. */
    int cols;               /* Number of cols. */
    int start_x;
    int start_y;
} maze_t;

/* Function prototypes. */
void maze_init(maze_t *maze, int cols, int rows, int start_x, int start_y, int goal_x, int goal_y);

void maze_destroy(maze_t *maze);

void maze_file_init(maze_file_t *file, char *filename);

void maze_file_destroy(maze_file_t *file);

#endif
