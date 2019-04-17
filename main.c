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
#include "sys/sysinfo.h"

#include "heap.h"
#include "node.h"
#include "maze.h"
#include "compass.h"    /* The heuristic. */

#define hash_distribute(num, x, y)      (((x) + (y)) % num)


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
    size_t thread_num;
    size_t *finished;
} a_star_argument_t;

typedef struct hda_message_t {
    node_t *parent;
    int x;
    int y;
    int gs;
    struct hda_message_t *next;
} hda_message_t;

typedef struct hda_mq_t {
    pthread_mutex_t mutex;
    hda_message_t *head;
} hda_mq_t;

typedef struct hda_argument_t {
    const maze_file_t *file;
    const maze_t *other_maze;
    maze_t *maze;
    pthread_mutex_t *return_value_mutex;
    a_star_return_t *return_value;
    size_t thread_num;
    size_t thread_id;
    hda_mq_t *mqs;
    size_t *msg_sent, *msg_received;
    size_t *finished;
} hda_argument_t;

void hda_mq_init(hda_mq_t *mq) {
    pthread_mutex_init(&mq->mutex, NULL);
    mq->head = NULL;
}

void hda_mq_destroy(hda_mq_t *mq) {
    hda_message_t *msg, *next_msg;
    for (msg = mq->head; msg != NULL; msg = next_msg) {
        next_msg = msg->next;
        free(msg);
    }
}

void *hda_star_search(hda_argument_t *arguments) {
    mem_pool_t mem_pool;
    heap_t heap;
    node_t *node = NULL, *other_node = NULL;
    hda_message_t *msg = NULL, *next_msg = NULL;
    int x_axis[4] = {0, 0, 0, 0};
    int y_axis[4] = {0, 0, 0, 0};
    int gs = 0;
    size_t i = 0;
    size_t id;
    hda_mq_t *mq;
    /* init and set up cleanups. */
    mem_pool_init(&mem_pool);
    heap_init(&heap);
    /* add start. */
    id = hash_distribute(arguments->thread_num, arguments->maze->start_x, arguments->maze->start_y);
    if (id == arguments->thread_id) {
        /* initialize first node. */
        ++arguments->msg_sent[arguments->thread_id];
        node = node_init(alloc_node(&mem_pool), arguments->maze->start_x, arguments->maze->start_y);
        node->parent = NULL;
        node->gs = 1;
        node->fs = 1 + heuristic(node, get_goal(arguments->maze));
        /* modify maze.nodes. */
        maze_node(arguments->maze, arguments->maze->start_x, arguments->maze->start_y) = node;
        /* insert first node. */
        heap_insert(&heap, node);
    }

    /* main loop. */
    while (!*arguments->finished) {
        if (heap.size > 1) {
            /* if there are nodes in heap. */
            node = heap_extract(&heap);
            /* if the node is worse than currently found best path */
            if (node->gs >= arguments->return_value->min_len) {
                /* dump heap and add number of nodes to message received. */
                arguments->msg_received[arguments->thread_id] += heap.size;
                heap.size = 1;
                continue;
            }
            /* if the node is opened in another list. */
            other_node = maze_node(arguments->other_maze, node->x, node->y);
            if (other_node != NULL) {
                int last_len, len = node->gs + other_node->gs;
                /* update current best path. */
                assert(!pthread_mutex_lock(arguments->return_value_mutex));
                last_len = arguments->return_value->min_len;
                if (len < last_len) {
                    arguments->return_value->min_len = len;
                    arguments->return_value->x = node->x;
                    arguments->return_value->y = node->y;
                }
                assert(!pthread_mutex_unlock(arguments->return_value_mutex));
            } else {
                /* initial four direction. */
                x_axis[0] = node->x + 1;
                y_axis[0] = node->y;
                x_axis[1] = node->x - 1;
                y_axis[1] = node->y;
                x_axis[2] = node->x;
                y_axis[2] = node->y + 1;
                x_axis[3] = node->x;
                y_axis[3] = node->y - 1;
                gs = node->gs + 1;
                /* Check all the neighbours. */
                for (i = 0; i < 4; ++i) {
                    /* send if not wall. */
                    if (maze_lines(arguments->file, x_axis[i], y_axis[i]) != '#') {
                        hda_message_t *new_msg = NULL;
                        /* pick msg queue. */
                        id = hash_distribute(arguments->thread_num, x_axis[i], y_axis[i]);
                        mq = &arguments->mqs[id];
                        /* allocate new message. */
                        new_msg = malloc(sizeof(hda_message_t));
                        new_msg->parent = node;
                        new_msg->x = x_axis[i];
                        new_msg->y = y_axis[i];
                        new_msg->gs = gs;
                        /* message sent add one */
                        ++arguments->msg_sent[arguments->thread_id];
                        /* send message. */
                        assert(!pthread_mutex_lock(&mq->mutex));
                        new_msg->next = mq->head;
                        mq->head = new_msg;
                        assert(!pthread_mutex_unlock(&mq->mutex));
                    }
                }
            }
            /* message received add one */
            ++arguments->msg_received[arguments->thread_id];
        } else {
            /* no nodes in heap. */
            while (arguments->mqs[arguments->thread_id].head == NULL) {
                int msg_sent_sum = 0, msg_received_sum = 0;
                /* barrier hit. If end, thread will stuck here and wait for cancel. */
                for (i = 0; i < arguments->thread_num; i++)
                    msg_received_sum += arguments->msg_received[i];
                for (i = 0; i < arguments->thread_num; i++)
                    msg_sent_sum += arguments->msg_sent[i];
                if (*arguments->finished ||
                    (arguments->return_value->min_len < INT_MAX && msg_sent_sum == msg_received_sum)) {
                    *arguments->finished = 1;
                    goto hda_star_search_end;
                }
            }
        }
        /* receive message. */
        assert(!pthread_mutex_lock(&arguments->mqs[arguments->thread_id].mutex));
        msg = arguments->mqs[arguments->thread_id].head;
        arguments->mqs[arguments->thread_id].head = NULL;
        assert(!pthread_mutex_unlock(&arguments->mqs[arguments->thread_id].mutex));
        /* add all nodes in message queue. */
        for (; msg != NULL; msg = next_msg) {
            node_t **adj_ptr = &maze_node(arguments->maze, msg->x, msg->y);
            node_t *adj = *adj_ptr;
            /* if NULL, the node is not opened */
            if (adj == NULL) {
                /* allocate new node and modify maze.nodes. */
                adj = node_init(alloc_node(&mem_pool), msg->x, msg->y);
                *adj_ptr = adj;
            }

            /* update if improved. */
            if (msg->gs < adj->gs) {
                /* modify node. */
                adj->parent = msg->parent;
                adj->gs = msg->gs;
                adj->fs = adj->gs + heuristic(adj, get_goal(arguments->maze));
                if (adj->heap_id != 0) {
                    heap_update(&heap, adj);
                    ++arguments->msg_received[arguments->thread_id];
                } else {
                    heap_insert(&heap, adj);
                }
            } else {
                ++arguments->msg_received[arguments->thread_id];
            }
            next_msg = msg->next;
            free(msg);
        }
    }

    hda_star_search_end:
    mem_pool_destroy(&mem_pool);
    heap_destroy(&heap);
    return NULL;
}

void *a_star_search(a_star_argument_t *arguments) {
    pthread_t *threads = NULL;
    hda_mq_t *message_queue = NULL;
    size_t *msg_sent = NULL;
    size_t *msg_received = NULL;
    hda_argument_t *args_for_threads = NULL;
    size_t i = 0;
    /* initialize and set up cleanups. */
    threads = malloc(arguments->thread_num * sizeof(pthread_t));
    message_queue = malloc(arguments->thread_num * sizeof(hda_mq_t));
    msg_sent = malloc(arguments->thread_num * sizeof(size_t));
    msg_received = malloc(arguments->thread_num * sizeof(size_t));
    args_for_threads = malloc(arguments->thread_num * sizeof(hda_argument_t));
    /* initialize thread each variables. */
    for (i = 0; i < arguments->thread_num; i++) {
        hda_mq_init(message_queue + i);
        msg_sent[i] = 0;
        msg_received[i] = 0;
        args_for_threads[i].file = arguments->file;
        args_for_threads[i].other_maze = arguments->other_maze;
        args_for_threads[i].maze = arguments->maze;
        args_for_threads[i].return_value_mutex = arguments->return_value_mutex;
        args_for_threads[i].return_value = arguments->return_value;
        args_for_threads[i].thread_num = arguments->thread_num;
        args_for_threads[i].thread_id = i;
        args_for_threads[i].mqs = message_queue;
        args_for_threads[i].msg_sent = msg_sent;
        args_for_threads[i].msg_received = msg_received;
        args_for_threads[i].finished = arguments->finished;
    }

    /* launch threads. */
    for (i = 0; i < arguments->thread_num; i++)
        assert(!pthread_create(threads + i, NULL, (void *(*)(void *)) hda_star_search, args_for_threads + i));
    /* join all the threads. */
    for (i = 0; i < arguments->thread_num; i++)
        assert(!pthread_join(threads[i], NULL));
    for (i = 0; i < arguments->thread_num; i++)
        hda_mq_destroy(message_queue + i);
    free(threads);
    free(message_queue);
    free(msg_sent);
    free(msg_received);
    free(args_for_threads);
    return NULL;
}

/**
 * Entrance point. Time ticking will be performed on the whole procedure,
 *   including I/O. Parallel and optimize as much as you can.
 */
int main(int argc, char *argv[]) {
    maze_file_t file;
    maze_t from_start_maze, from_goal_maze;
    pthread_mutex_t return_value_mutex = PTHREAD_MUTEX_INITIALIZER;
    a_star_argument_t argument_start, argument_goal;
    pthread_t from_start, from_goal;
    node_t *node = NULL;
    a_star_return_t return_value = {-1, -1, INT_MAX};
    size_t thread_num = get_nprocs();
    size_t finished = 0;
    int count = 1;

    /* Must have given the source file name. */
    assert(argc == 2);
    /* Initializations. */
    maze_file_init(&file, argv[1]);
    maze_init(&from_start_maze, file.cols, file.rows, 1, 1, file.cols - 1, file.rows - 2);
    maze_init(&from_goal_maze, file.cols, file.rows, file.cols - 2, file.rows - 2, 0, 1);
    /* shared arguments-> */
    argument_start.file = &file;
    argument_goal.file = &file;
    argument_start.other_maze = &from_goal_maze;
    argument_goal.other_maze = &from_start_maze;
    argument_start.maze = &from_start_maze;
    argument_goal.maze = &from_goal_maze;
    argument_start.return_value_mutex = &return_value_mutex;
    argument_goal.return_value_mutex = &return_value_mutex;
    argument_start.return_value = &return_value;
    argument_goal.return_value = &return_value;
    argument_start.thread_num = thread_num / 2;
    argument_goal.thread_num = thread_num / 2;
    argument_start.finished = &finished;
    argument_goal.finished = &finished;

    /* create two threads. */
    assert(!pthread_create(&from_start, NULL, (void *(*)(void *)) a_star_search, &argument_start));
    assert(!pthread_create(&from_goal, NULL, (void *(*)(void *)) a_star_search, &argument_goal));
    /* join any of the two thread. */
    /* join two threads thread. */
    assert(!pthread_join(from_start, NULL));
    assert(!pthread_join(from_goal, NULL));

    /* Print the steps back. */
    maze_lines(&file, return_value.x, return_value.y) = '*';
    node = maze_node(argument_start.maze, return_value.x, return_value.y)->parent;
    while (node != NULL) {
        maze_lines(&file, node->x, node->y) = '*';
        node = node->parent;
        count++;
    }
    node = maze_node(argument_goal.maze, return_value.x, return_value.y)->parent;
    while (node != NULL) {
        maze_lines(&file, node->x, node->y) = '*';
        node = node->parent;
        count++;
    }
    printf("%d\n", count);
    /* Free resources and return. */
    maze_destroy(argument_start.maze);
    maze_destroy(argument_goal.maze);
    maze_file_destroy(&file);
    return 0;
}
