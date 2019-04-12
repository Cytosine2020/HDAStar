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
#include <bits/sigthread.h>
#include <zconf.h>

#include "heap.h"
#include "node.h"
#include "maze.h"
#include "compass.h"    /* The heuristic. */


typedef struct a_star_return_t {
    int x;
    int y;
    int min_len;
    int upper_bound;
    pthread_t self;
} a_star_return_t;

typedef struct a_star_argument_t {
    const maze_file_t *file;
    const maze_t *other_maze;
    const heap_t *other_heap;
    mem_pool_t *mem_pool;
    maze_t *maze;
    heap_t *heap;
    pthread_cond_t *finished;
    pthread_mutex_t *finished_mutex;
    a_star_return_t *return_value;
} a_star_argument_t;

void *a_star_search(void *arguments) {
    int x_axis[4] = {0, 0, 0, 0};
    int y_axis[4] = {0, 0, 0, 0};
    a_star_argument_t data = *(a_star_argument_t *) arguments;
    node_t *node = NULL, *other_node = NULL;
    int i = 0, offset = 0;
    assert(!pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL));
    /* Loop and repeatedly extracts the node with the highest f-score to process on. */
    while (data.heap->size > 1) {
        node = heap_extract(data.heap);
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
            node_t **adj_ptr = &maze_node(data.maze, x_axis[i], y_axis[i]);
            node_t *adj = *adj_ptr;
            /* if NULL, the node is not opened */
            if (adj == NULL) {
                /* read file and judge whether it is wall. */
                if (maze_lines(data.file, x_axis[i], y_axis[i]) == '#') {
                    /* modify maze.nodes. */
                    *adj_ptr = get_wall(data.maze);
                } else {
                    /* allocate new node and modify maze.nodes. */
                    adj = node_init(alloc_node(data.mem_pool), x_axis[i], y_axis[i]);
                    *adj_ptr = adj;
                    /* initialize node. */
                    adj->parent = node;
                    adj->gs = node->gs + 1;
                    adj->fs = adj->gs + heuristic(adj, get_goal(data.maze));
                    /* update node. */
                    heap_insert(data.heap, adj);
                }
            } else {
                /* update if not wall, not closed and can improve. */
                if (!is_wall(data.maze, adj) && !adj->closed && node->gs + 1 < adj->gs) {
                    /* initialize node. */
                    adj->parent = node;
                    adj->gs = node->gs + 1;
                    adj->fs = adj->gs + heuristic(adj, get_goal(data.maze));
                    /* update node. */
                    heap_update(data.heap, adj);
                }
            }
        }
        /* close node, */
        offset = node->y * data.maze->cols + node->x;
        data.maze->nodes[offset]->closed = true;
        other_node = data.other_maze->nodes[offset];
        if (other_node != NULL && !is_wall(data.other_maze, other_node)) {
            int last_len, len = node->gs + other_node->gs;
            int this_heap_least, other_heap_least, max_bound, last_bound;
            if (data.heap->size > 1) this_heap_least = data.heap->nodes[1]->fs;
            else this_heap_least = INT_MAX;
            if (data.other_heap->size > 1) other_heap_least = data.other_heap->nodes[1]->fs;
            else other_heap_least = INT_MAX;
            max_bound = this_heap_least > other_heap_least ? this_heap_least : other_heap_least;
            assert(!pthread_mutex_lock(data.finished_mutex));
            last_len = data.return_value->min_len;
            if (len < last_len) {
                data.return_value->min_len = len;
                data.return_value->x = node->x;
                data.return_value->y = node->y;
            }
            last_bound = data.return_value->upper_bound;
            if (max_bound < last_bound) data.return_value->upper_bound = max_bound;
            if (data.return_value->upper_bound > data.return_value->min_len) {
                assert(!pthread_mutex_unlock(data.finished_mutex));
                break;
            }
            assert(!pthread_mutex_unlock(data.finished_mutex));
        }
    }
    /* return to main thread. */
    assert(!pthread_mutex_lock(data.finished_mutex));
    data.return_value->self = pthread_self();
    assert(!pthread_cond_signal(data.finished));
    assert(!pthread_mutex_unlock(data.finished_mutex));
    return NULL;
}

/**
 * Entrance point. Time ticking will be performed on the whole procedure,
 *   including I/O. Parallel and optimize as much as you can.
 */
int main(int argc, char *argv[]) {
    maze_file_t *file = NULL;
    pthread_cond_t finished = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t finished_mutex = PTHREAD_MUTEX_INITIALIZER;
    a_star_argument_t argument_start, argument_goal;
    pthread_t from_start, from_goal;
    node_t *node = NULL;
    a_star_return_t return_value = {-1, -1, INT_MAX, -1, -1};
    int count = 1;

    assert(argc == 2);  /* Must have given the source file name. */
    file = maze_file_init(argv[1]);

    /* Initializations. */
    argument_start.mem_pool = init_pool();
    argument_start.maze = maze_init(file->cols, file->rows, file->cols - 1, file->rows - 2);
    argument_start.heap = heap_init();
    /* initialize first node. */
    node = node_init(alloc_node(argument_start.mem_pool), 1, 1);
    node->parent = NULL;
    node->gs = 1;
    node->fs = 1 + heuristic(node, get_goal(argument_start.maze));
    /* modify maze.nodes. */
    maze_node(argument_start.maze, 1, 1) = node;
    /* insert first node. */
    heap_insert(argument_start.heap, node);

    /* Initializations. */
    argument_goal.mem_pool = init_pool();
    argument_goal.maze = maze_init(file->cols, file->rows, 1, 0);
    argument_goal.heap = heap_init();
    /* initialize first node. */
    node = node_init(alloc_node(argument_goal.mem_pool), file->cols - 2, file->rows - 2);
    node->parent = NULL;
    node->gs = 1;
    node->fs = 1 + heuristic(node, get_goal(argument_goal.maze));
    /* modify maze.nodes. */
    maze_node(argument_goal.maze, file->cols - 2, file->rows - 2) = node;
    /* insert first node. */
    heap_insert(argument_goal.heap, node);

    /* shared data. */
    argument_start.file = file;
    argument_goal.file = file;
    argument_start.other_maze = argument_goal.maze;
    argument_goal.other_maze = argument_start.maze;
    argument_start.other_heap = argument_goal.heap;
    argument_goal.other_heap = argument_start.heap;
    argument_start.finished = &finished;
    argument_goal.finished = &finished;
    argument_start.finished_mutex = &finished_mutex;
    argument_goal.finished_mutex = &finished_mutex;
    argument_start.return_value = &return_value;
    argument_goal.return_value = &return_value;


    assert(!pthread_mutex_lock(&finished_mutex));
    /* create two threads. */
    assert(!pthread_create(&from_start, NULL, a_star_search, &argument_start));
    assert(!pthread_create(&from_goal, NULL, a_star_search, &argument_goal));
    /* join any of the two thread. */
    assert(!pthread_cond_wait(&finished, &finished_mutex));
    /* kill another thread. */
    if (!pthread_equal(from_start, return_value.self))
        assert(!pthread_cancel(from_start));
    if (!pthread_equal(from_goal, return_value.self))
        assert(!pthread_cancel(from_goal));

    assert(!pthread_join(from_start, NULL));
    assert(!pthread_join(from_goal, NULL));
    assert(!pthread_mutex_unlock(&finished_mutex));

    /* Print the steps back. */
    maze_lines(file, return_value.x, return_value.y) = '*';
    node = maze_node(argument_start.maze, return_value.x, return_value.y)->parent;
    while (node != NULL) {
        maze_lines(file, node->x, node->y) = '*';
        node = node->parent;
        count++;
    }
    node = maze_node(argument_goal.maze, return_value.x, return_value.y)->parent;
    while (node != NULL) {
        maze_lines(file, node->x, node->y) = '*';
        node = node->parent;
        count++;
    }
    printf("%d\n", count);
    /* Free resources and return. */
    heap_destroy(argument_start.heap);
    heap_destroy(argument_goal.heap);
    maze_destroy(argument_start.maze);
    maze_destroy(argument_goal.maze);
    release_pool(argument_start.mem_pool);
    release_pool(argument_goal.mem_pool);
    maze_file_destroy(file);
    return 0;
}
