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
#include <unistd.h>
#include <memory.h>

#include "maze.h"
#include "node.h"

/**
 * Initialize a maze from a formatted source file named FILENAME. Returns the
 *   pointer to the new maze.
 */
maze_t *maze_init(int cols, int rows, int start_x, int start_y, int goal_x, int goal_y) {
    maze_t *maze = malloc(sizeof(maze_t));
    size_t size = rows * cols * sizeof(node_t *);
    /* Allocate space for all nodes (cells) inside the maze. */
    maze->cols = cols;
    maze->start_x = start_x;
    maze->start_y = start_y;
    maze->nodes = malloc(size);
    memset(maze->nodes, 0, size);
    /* initial special nodes. */
    node_init(&maze->goal, goal_x, goal_y);
    return maze;
}

/**
 * Delete the memory occupied by the maze M.
 */
void maze_destroy(maze_t *maze) {
    free(maze->nodes);
    free(maze);
}

maze_file_t *maze_file_init(char *filename) {
    char *file_ptr;
    struct stat status;
    int rows, cols;
    int i;
    maze_file_t *file = malloc(sizeof(maze_file_t));
    /* Open the source file and read in number of rows & cols. */
    file->fd = open(filename, O_RDWR);
    assert(file->fd != -1);
    assert(fstat(file->fd, &status) != -1);
    file->mem_size = (size_t) status.st_size;
    file->mem_map = mmap(
            NULL,
            (size_t) status.st_size,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            file->fd, 0);
    assert(file->mem_map != MAP_FAILED);

    /* at maze_print_step.*/
    assert(sscanf((char *) file->mem_map, "%d %d\n", &rows, &cols) == 2);
    file->cols = cols;
    file->rows = rows;

    /* initial lines. */
    file->lines = malloc(rows * sizeof(char *));
    assert(file->lines != NULL);
    memset(file->lines, 0, rows * sizeof(char *));

    file_ptr = file->mem_map;
    for (i = 0; i < rows; i++) {
        while (*(file_ptr++) != '\n');
        file->lines[i] = file_ptr;
        file_ptr += file->cols;
    }
    maze_lines(file, 0, 1) = '#';
    maze_lines(file, cols - 1, rows - 2) = '#';

    return file;
}

void maze_file_destroy(maze_file_t *file) {
    maze_lines(file, 0, 1) = '@';
    maze_lines(file, file->cols - 1, file->rows - 2) = '%';
    free(file->lines);
    msync(file->mem_map, file->mem_size, MS_ASYNC | MS_INVALIDATE);
    munmap(file->mem_map, file->mem_size);
    close(file->fd);
    free(file);
}
