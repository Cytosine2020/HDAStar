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
 * Initialize a maze from a formatted source file named FILENAME. Returns the
 *   pointer to the new maze.
 */
void maze_init(maze_t *maze, char *filename) {
    int rows, cols;
    size_t size;
    char *file_ptr;
    struct stat status;

    /* Open the source file and read in number of rows & cols. */
    maze->fd = open(filename, O_RDWR);
    assert(maze->fd != -1);
    assert(fstat(maze->fd, &status) != -1);
    maze->mem_size = (size_t) status.st_size;
    maze->mem_map = mmap(
            NULL,
            (size_t) status.st_size,
            PROT_READ | PROT_WRITE,
            MAP_SHARED | MAP_FILE,
            maze->fd, 0);
    assert(maze->mem_map != MAP_FAILED);

    /* at maze_print_step.*/
    assert(sscanf((char *) maze->mem_map, "%d %d\n", &rows, &cols) == 2);
    maze->rows = rows;
    maze->cols = cols;

    /* initial lines. */
    maze->lines = malloc(rows * sizeof(char *));
    assert(maze->lines != NULL);
    memset(maze->lines, 0, rows * sizeof(char *));

    file_ptr = maze->mem_map;
    while(*(file_ptr++) != '\n');
    maze->lines[0] = file_ptr;

    /* Allocate space for all nodes (cells) inside the maze. */
    size = rows * cols * sizeof(node_t *);
    maze->nodes = malloc(size);
    memset(maze->nodes, 0, size);

    node_init(&maze->start, 0, 1);
    node_init(&maze->goal, cols - 1, rows - 2);
    node_init(&maze->wall, -1, -1);

    maze->nodes[cols] = &maze->start;
    maze->nodes[(rows - 1) * cols - 1] = &maze->goal;
}

/**
 * Delete the memory occupied by the maze M.
 */
void maze_destroy(maze_t *maze) {
    free(maze->nodes);
    free(maze->lines);
    msync(maze->mem_map, maze->mem_size, MS_ASYNC | MS_INVALIDATE);
    munmap(maze->mem_map, maze->mem_size);
    close(maze->fd);
}

/**
 * Initialize a cell in maze M at position (X, Y) to be a non-WALL.
 */
node_t *maze_set_cell(maze_t *maze, int x, int y) {
    node_t *node;
    if (*get_char_loc(maze, x, y) == '#') node = &(maze->wall);
    else node = node_init(alloc_node(), x, y);
    return maze->nodes[y * maze->cols + x] = node;
}

/**
 * Get the cell in maze M at position (X, Y). Returns the pointer to the
 *   specified cell.
 */
node_t *maze_get_cell(maze_t *maze, int x, int y) {
    node_t *node = maze->nodes[y * maze->cols + x];
    if (node == NULL) node = maze_set_cell(maze, x, y);
    return node;
}

/**
 * Sets the position of node N in maze M in the source file to be "*",
 *   indicating it is an intermediate step along the shortest path.
 */
char *get_char_loc(maze_t *maze, int x, int y) {
    char * line = maze->lines[y];
    if (line == NULL) {
        int i;
        char *file_ptr;
        for (i = 0; maze->lines[i] == NULL; i++);
        file_ptr = maze->lines[i];
        for(i++; i <= y; i++) {
            file_ptr += maze->cols;
            while(*(file_ptr++) != '\n');
            maze->lines[i] = file_ptr;
        }
    }
    return maze->lines[y] + x;
}
