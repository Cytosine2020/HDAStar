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
#include <pthread.h>
#include <limits.h>

#include "heap.h"
#include "node.h"
#include "maze.h"
#include "compass.h"    /* The heuristic. */


typedef struct a_star_return_t {
    int x;
    int y;
    int min_len;
} a_star_return_t;

typedef struct a_star_argument_t {
    const maze_file_t *file;
    const maze_t *other_maze;
    maze_t *maze;
    pthread_mutex_t *return_value_mutex;
    a_star_return_t *return_value;
    size_t *finished;
} a_star_argument_t;

void *a_star_search(a_star_argument_t *arguments) {
    mem_pool_t pool;
    heap_t heap;
    int x_axis[4] = {0, 0, 0, 0};
    int y_axis[4] = {0, 0, 0, 0};
    node_t *node = NULL, *other_node = NULL;
    int i = 0;

    pool_init(&pool);
    heap_init(&heap);
    node = node_init(alloc_node(&pool), arguments->maze->start_x, arguments->maze->start_y);
    node->parent = NULL;
    node->gs = 1;
    node->fs = 1 + heuristic(node, get_goal(arguments->maze));
    /* modify maze.nodes. */
    maze_node(arguments->maze, node->x, node->y) = node;
    /* insert first node. */
    heap_insert(&heap, node);
    /* Loop and repeatedly extracts the node with the highest f-score to process on. */
    while (!*arguments->finished && heap.size > 1) {
        node = heap_extract(&heap);
        if (node->fs >= arguments->return_value->min_len) {
            node->closed = true;
            break;
        }
        other_node = maze_node(arguments->other_maze, node->x, node->y);
        if (other_node != NULL && other_node->closed) {
            int last_len, len = node->gs + other_node->gs;
            node->closed = true;
            assert(!pthread_mutex_lock(arguments->return_value_mutex));
            last_len = arguments->return_value->min_len;
            if (len < last_len) {
                arguments->return_value->min_len = len;
                arguments->return_value->x = node->x;
                arguments->return_value->y = node->y;
            }
            assert(!pthread_mutex_unlock(arguments->return_value_mutex));
            node->closed = true;
            continue;
        }
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
            node_t **adj_ptr = &maze_node(arguments->maze, x_axis[i], y_axis[i]);
            node_t *adj = *adj_ptr;
            if (maze_lines(arguments->file, x_axis[i], y_axis[i]) != '#') {
                /* if NULL, the node is not opened */
                if (adj == NULL) {
                    /* allocate new node and modify maze.nodes. */
                    adj = node_init(alloc_node(&pool), x_axis[i], y_axis[i]);
                    *adj_ptr = adj;
                    /* initialize node. */
                    adj->parent = node;
                    adj->gs = node->gs + 1;
                    adj->fs = adj->gs + heuristic(adj, get_goal(arguments->maze));
                    /* update node. */
                    heap_insert(&heap, adj);
                } else {
                    /* update if not wall, not closed and can improve. */
                    if (!adj->closed && node->gs + 1 < adj->gs) {
                        /* initialize node. */
                        adj->parent = node;
                        adj->gs = node->gs + 1;
                        adj->fs = adj->gs + heuristic(adj, get_goal(arguments->maze));
                        /* update node. */
                        heap_update(&heap, adj);
                    }
                }
            }
        }
        node->closed = true;
    }
    /* return to main thread. */
    if (arguments->return_value->min_len < INT_MAX)
        *arguments->finished = 1;
    heap_destroy(&heap);
    pool_destroy(&pool);
    return NULL;
}

/**
 * Entrance point. Time ticking will be performed on the whole procedure,
 *   including I/O. Parallel and optimize as much as you can.
 */
int main(int argc, char *argv[]) {
    maze_file_t *file = NULL;
    maze_t *maze_start = NULL, *maze_goal = NULL;
    pthread_mutex_t *return_value_mutex = NULL;
    a_star_return_t *return_value = NULL;
    size_t *finished;
    a_star_argument_t *argument_start = NULL, *argument_goal = NULL;
    pthread_t from_start, from_goal;
    node_t *node = NULL;
    int count = 1;

    /* Must have given the source file name. */
    assert(argc == 2);
    file = maze_file_init(argv[1]);
    maze_start = maze_init(file->cols, file->rows, 1, 1, file->cols - 1, file->rows - 2);
    maze_goal = maze_init(file->cols, file->rows, file->cols - 2, file->rows - 2, 0, 1);
    return_value_mutex = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(return_value_mutex, NULL);
    return_value = malloc(sizeof(a_star_return_t));
    return_value->min_len = INT_MAX;
    return_value->x = -1;
    return_value->y = -1;
    finished = malloc(sizeof(size_t));
    *finished = 0;

    argument_start = malloc(sizeof(a_star_argument_t));
    argument_goal = malloc(sizeof(a_star_argument_t));
    /* shared arguments-> */
    argument_start->file = file;
    argument_goal->file = file;
    argument_start->other_maze = maze_goal;
    argument_goal->other_maze = maze_start;
    argument_start->maze = maze_start;
    argument_goal->maze = maze_goal;
    argument_start->return_value = return_value;
    argument_goal->return_value = return_value;
    argument_start->return_value_mutex = return_value_mutex;
    argument_goal->return_value_mutex = return_value_mutex;
    argument_start->finished = finished;
    argument_goal->finished = finished;

    /* create two threads. */
    assert(!pthread_create(&from_start, NULL, (void *(*)(void *)) a_star_search, argument_start));
    assert(!pthread_create(&from_goal, NULL, (void *(*)(void *)) a_star_search, argument_goal));
    /* join two threads thread. */
    assert(!pthread_join(from_start, NULL));
    assert(!pthread_join(from_goal, NULL));

    /* Print the steps back. */
    maze_lines(file, return_value->x, return_value->y) = '*';
    node = maze_node(argument_start->maze, return_value->x, return_value->y)->parent;
    while (node != NULL) {
        maze_lines(file, node->x, node->y) = '*';
        node = node->parent;
        count++;
    }
    node = maze_node(argument_goal->maze, return_value->x, return_value->y)->parent;
    while (node != NULL) {
        maze_lines(file, node->x, node->y) = '*';
        node = node->parent;
        count++;
    }
    printf("%d\n", count);
    /* Free resources and return. */
    maze_file_destroy(file);
    maze_destroy(argument_start->maze);
    maze_destroy(argument_goal->maze);
    free(return_value_mutex);
    free(return_value);
    free(finished);
    free(argument_start);
    free(argument_goal);
    return 0;
}
