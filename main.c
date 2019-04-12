/**
 * File: main.c
 *
 *   Main body of A* path searching algorithm on a block maze. The maze is
 *     given in a source file, whose name is put as a command-line argument.
 *     Then it should perform an A* search on the maze and prints the steps
 *     along the computed shortest path back to the file.
 *
 *     * Heuristic function chosen as the "Manhattan Distance":
 *
 *         heuristic(n1, n2) = |n1.x - n2.x| + |n1.y - n2.y|
 *
 *     * The whole procedure, including I/O and computing, will be time-
 *         ticked. So you are allowed to modify and optimize everything in
 *         this file and in all the libraries, as long as it satisfies:
 *
 *         1. It performs an A* path searching algorithm on the given maze.
 *
 *         2. It computes one SHORTEST (exactly optimal) path and prints the
 *              steps along the shortest path back to file, just as the
 *              original version.
 *
 *         3. Compiles with the given "Makefile". That means we are using
 *              (and only manually using) "pthread" for parallelization.
 *              Further parallelization techniques, such as OpenMP and SSE,
 *              are not required (and not allowed).
 *
 *         4. If there are multiple shortest paths, any one of them will be
 *              accepted. Please make sure you only print exactly one valid
 *              path to the file.
 *
 * Jose @ ShanghaiTech University
 */

#include <stdlib.h>     /* NULL */
#include <assert.h>     /* assert */
#include "heap.h"
#include "node.h"
#include "maze.h"
#include "compass.h"    /* The heuristic. */

/**
 * Entrance point. Time ticking will be performed on the whole procedure,
 *   including I/O. Parallel and optimize as much as you can.
 */
int main(int argc, char *argv[]) {
    maze_file_t *file = NULL;
    mem_pool_t *mem_pool = NULL;
    maze_t *maze = NULL;
    heap_t *heap = NULL;
    node_t *node = NULL;
    int x_axis[4] = {0, 0, 0, 0};
    int y_axis[4] = {0, 0, 0, 0};
    int count = 0, i = 0;

    assert(argc == 2);  /* Must have given the source file name. */

    /* Initializations. */
    file = maze_file_init(argv[1]);
    mem_pool = init_pool();
    maze = maze_init(file->rows, file->cols);
    heap = heap_init();
    /* initialize first node. */
    node = node_init(alloc_node(mem_pool), 1, 1);
    node->parent = NULL;
    node->gs = 1;
    node->fs = 1 + heuristic(node, get_goal(maze));
    /* modify maze.nodes. */
    maze_node(maze, 1, 1) = node;
    /* update node. */
    heap_insert(heap, node);

    /* Loop and repeatedly extracts the node with the highest f-score to
       process on. */
    while (heap->size > 1) {
        node = heap_extract(heap);
        /* Goal point reached. */
        if (is_goal(maze, node)) break;
        /* close node, */
        maze_node(maze, node->x, node->y) = get_closed(maze);
        /* initial four direction. */
        x_axis[0] = node->x + 1;
        y_axis[0] = node->y;
        x_axis[1] = node->x - 1;
        y_axis[1] = node->y;
        x_axis[2] = node->x;
        y_axis[2] = node->y + 1;
        x_axis[3] = node->x;
        y_axis[3] = node->y - 1;
        /* Check all the neighbours. */
        for (i = 0; i < 4; ++i) {
            node_t **adj_ptr = &maze_node(maze, x_axis[i], y_axis[i]);
            node_t *adj = *adj_ptr;

            if (is_closed(maze, adj)) continue;
            /* if NULL, the node is not opened */
            if (adj == NULL) {
                /* read file and judge whether it is wall. */
                if (maze_lines(file, x_axis[i], y_axis[i]) == '#') {
                    /* modify maze.nodes. */
                    *adj_ptr = get_wall(maze);
                } else {
                    /* allocate new node and modify maze.nodes. */
                    adj = node_init(alloc_node(mem_pool), x_axis[i], y_axis[i]);
                    *adj_ptr = adj;
                    /* initialize node. */
                    adj->parent = node;
                    adj->gs = node->gs + 1;
                    adj->fs = adj->gs + heuristic(adj, get_goal(maze));
                    /* update node. */
                    heap_insert(heap, adj);
                }
            } else {
                /* update if not wall, not closed and can improve. */
                if (!is_wall(maze, adj) && node->gs + 1 < adj->gs) {
                    /* initialize node. */
                    adj->parent = node;
                    adj->gs = node->gs + 1;
                    adj->fs = adj->gs + heuristic(adj, get_goal(maze));
                    /* update node. */
                    heap_update(heap, adj);
                }
            }
        }
    }

    /* Print the steps back. */
    node = get_goal(maze)->parent;
    while (node != NULL) {
        maze_lines(file, node->x, node->y) = '*';
        node = node->parent;
        count++;
    }
    printf("%d\n", count);
    /* Free resources and return. */
    heap_destroy(heap);
    maze_destroy(maze);
    release_pool(mem_pool);
    maze_file_destroy(file);
    return 0;
}
