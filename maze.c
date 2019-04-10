/**
 * File: maze.c
 * 
 *   Implementation of library functions manipulating a minecraft-style block
 *     maze. Feel free to add, remove, or modify these library functions to
 *     serve your algorithm.
 *  
 * Jose @ ShanghaiTech University
 */

#include <stdlib.h>     /* abs, malloc, free */
#include <assert.h>     /* assert */
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <memory.h>
#include <zconf.h>

#include "maze.h"
#include "node.h"

/**
 * Maps a character C to its corresponding mark type. Returns the mark.
 */
static mark_t char_to_mark(char character) {
    switch (character) {
        case '#':
            return WALL;
        case '@':
            return START;
        case '%':
            return GOAL;
        default:
            return NONE;
    }
}

static char *next_line(char *ptr) {
    int i = 0;
    for (i = 0; ptr[i] != '\n'; i++);
    return ptr + i + 1;
}

/**
 * Initialize a maze from a formatted source file named FILENAME. Returns the
 *   pointer to the new maze.
 */
maze_t *maze_init(char *filename) {
    int rows, cols, i, j, character;
    maze_t *maze = malloc(sizeof(maze_t));
    struct stat status;
    char *file_ptr, *file_ptr_end;

    /* Open the source file and read in number of rows & cols. */
    maze->fd = open(filename, O_RDWR);
    assert(maze->fd != -1);
    assert(fstat(maze->fd, &status) != -1);
    maze->memsize = (size_t) status.st_size;
    maze->memmap = mmap(NULL, (size_t) status.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, maze->fd, 0);

    /* at maze_print_step.*/
    assert(sscanf((char *) maze->memmap, "%d %d\n", &rows, &cols) == 2);
    maze->rows = rows;
    maze->cols = cols;
    maze->lines = malloc(rows * sizeof(char *));
    assert(maze->lines != NULL);
    memset(maze->lines, 0, rows * sizeof(char *));

    file_ptr = next_line(maze->memmap);
    file_ptr_end = (char *) maze->memmap + status.st_size;

    /* Allocate space for all nodes (cells) inside the maze, then read them
       in from the source file. The maze records all pointers to its nodes
       in an array NODES. */
    maze->nodes = malloc(rows * cols * sizeof(node_t *));
    for (i = 0; i < rows; ++i) {
        maze->lines[i] = file_ptr;
        for (j = 0; j < cols; ++j) {
            mark_t mark = char_to_mark(*(file_ptr++));
            maze_set_cell(maze, i, j, mark);
            if (mark == START)
                maze->start = maze_get_cell(maze, i, j);
            else if (mark == GOAL)
                maze->goal = maze_get_cell(maze, i, j);
        }
        character = '\0';
        while (character != '\n' && file_ptr < file_ptr_end)
            character = *(file_ptr++);
    }

    return maze;
}

/**
 * Delete the memory occupied by the maze M.
 */
void maze_destroy(maze_t *maze) {
    int i, j;
    for (i = 0; i < maze->rows; ++i)
        for (j = 0; j < maze->cols; ++j)
            free(maze->nodes[i * maze->cols + j]);
    free(maze->nodes);
    free(maze->lines);
    msync(maze->memmap, maze->memsize, MS_ASYNC);
    munmap(maze->memmap, maze->memsize);
    close(maze->fd);
    free(maze);
}

/**
 * Initialize a cell in maze M at position (X, Y) to be a MARK-type cell.
 */
void maze_set_cell(maze_t *maze, int x, int y, mark_t mark) {
    assert(x >= 0 && x < maze->rows && y >= 0 && y < maze->cols);
    maze->nodes[x * maze->cols + y] = node_init(x, y, mark);
}

/**
 * Get the cell in maze M at position (X, Y). Returns the pointer to the
 *   specified cell.
 */
node_t *maze_get_cell(maze_t *maze, int x, int y) {
    if (x < 0 || x >= maze->rows || y < 0 || y >= maze->cols) /* Returns NULL if exceeds boundary. */
        return NULL;
    return maze->nodes[x * maze->cols + y];   /* Notice the row-major order. */
}

/**
 * Sets the position of node N in maze M in the source file to be "*",
 *   indicating it is an intermediate step along the shortest path.
 */
void maze_print_step(maze_t *maze, node_t *node) {
    maze->lines[node->x][node->y] = '*';
}
