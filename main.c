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

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#include <stdlib.h>     /* NULL */
#include <assert.h>     /* assert */
#include <pthread.h>
#include <limits.h>
#include <sys/mman.h>
#include <sys/sysinfo.h>

#include "heap.h"
#include "node.h"
#include "maze.h"
#include "compass.h"    /* The heuristic. */

#define hash_distribute(num, x, y)      (((x) + (y)) % num)
#define MSG_MEM_MAP_SIZE (0x10000)

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
    hda_message_t *head;
    void *start_chunk;
    void *end_chunk;
    hda_message_t *end_chunk_cap;
    hda_message_t *end_chunk_len;
    hda_message_t *bin;
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
    mq->head = NULL;
    mq->start_chunk = mmap(
            NULL,
            MSG_MEM_MAP_SIZE,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS,
            -1, 0);
    assert(mq->start_chunk != MAP_FAILED);
    assert(mq->start_chunk != MAP_FAILED);
    mq->end_chunk = mq->start_chunk;
    mq->end_chunk_len = (hda_message_t *) mq->end_chunk + 1;
    mq->end_chunk_cap = (hda_message_t *) ((size_t) mq->end_chunk + MSG_MEM_MAP_SIZE) - 1;
    *(void **) mq = NULL;
    mq->bin = NULL;
}

hda_message_t *alloc_msg(hda_mq_t *mq) {
    if (mq->bin != NULL) {
        hda_message_t *tmp = mq->bin;
        mq->bin = tmp->next;
        return tmp;
    } else {
        if (mq->end_chunk_len > mq->end_chunk_cap) {
            void *new_chunk = mmap(
                    (void *) ((size_t) mq->end_chunk + MSG_MEM_MAP_SIZE),
                    MSG_MEM_MAP_SIZE,
                    PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS,
                    -1, 0);
            assert(new_chunk != MAP_FAILED);
            *(void **) mq->end_chunk = new_chunk;
            mq->end_chunk = new_chunk;
            mq->end_chunk_len = (hda_message_t *) new_chunk + 1;
            mq->end_chunk_cap = (hda_message_t *) ((size_t) new_chunk + MSG_MEM_MAP_SIZE) - 1;
            *(void **) new_chunk = NULL;
        }
        return mq->end_chunk_len++;
    }
}

void free_msg(hda_mq_t *mq, hda_message_t *msg) {
    msg->next = mq->bin;
    mq->bin = msg;
}

void hda_mq_destroy(hda_mq_t *mq) {
    void *this_pool = mq->start_chunk;
    while (this_pool != NULL) {
        void *next_pool = *(void **) this_pool;
        munmap(this_pool, MSG_MEM_MAP_SIZE);
        this_pool = next_pool;
    }
}

void hda_mq_send(hda_mq_t *mq, hda_message_t *msg) {
    __asm__ __volatile__(
    "       mov         %E[ptr], %%rax;     "
    "hda_mq_send_loop:                      "
    "       mov         %%rax, %E[next];    "
    "lock   cmpxchg     %[msg], %E[ptr];    "
    "       jne         hda_mq_send_loop;   "
    :
    :[ptr] "r"(&mq->head), [next] "r"(&msg->next), [msg] "r"(msg)
    :"rax", "cc", "memory");
}

hda_message_t *hda_mq_receive(hda_mq_t *mq) {
    hda_message_t *msg = NULL;

    __asm__ __volatile__(
    "lock   xchg        %[msg], %E[ptr];    "
    :[msg] "+r"(msg)
    :[ptr] "r"(&mq->head)
    : "memory");

    return msg;
}

void update_return_value(pthread_mutex_t *mutex, a_star_return_t *return_value, int len, int x, int y) {
    assert(!pthread_mutex_lock(mutex));
    if (len < return_value->min_len) {
        return_value->min_len = len;
        return_value->x = x;
        return_value->y = y;
    }
    assert(!pthread_mutex_unlock(mutex));
}

void open_node(hda_argument_t *args, hda_mq_t *msg_queue, node_t *parent, int x, int y, size_t *msg_sent) {
    if (maze_lines(args->file, x, y) != '#') {
        node_t *node = maze_node(args->maze, x, y);
        if (node == NULL || parent->gs + 1 < node->gs) {
            hda_message_t *msg = alloc_msg(msg_queue);
            msg->parent = parent;
            msg->x = x;
            msg->y = y;
            msg->gs = parent->gs + 1;
            /* message sent add one */
            ++*msg_sent;
            /* send message. */
            hda_mq_send(&args->mqs[hash_distribute(args->thread_num, x, y)], msg);
        }
    }
}

size_t detect_termination(hda_argument_t *args, hda_mq_t *msg_queue) {
    while (msg_queue->head == NULL) {
        size_t msg_sent_sum = 0, msg_received_sum = 0, i;
        /* barrier hit. If end, thread will stuck here and wait for cancel. */
        for (i = 0; i < args->thread_num; i++)
            msg_received_sum += args->msg_received[i];
        for (i = 0; i < args->thread_num; i++)
            msg_sent_sum += args->msg_sent[i];
        if (*args->finished ||
            (args->return_value->min_len < INT_MAX && msg_sent_sum == msg_received_sum)) {
            *args->finished = 1;
            return 1;
        }
    }
    return 0;
}

void receive_msg(hda_argument_t *args, hda_mq_t *msg_queue, mem_pool_t *pool, heap_t *heap, size_t *msg_received) {
    hda_message_t *msg, *next_msg;
    /* receive message. */
    msg = hda_mq_receive(msg_queue);
    /* add all nodes in message queue. */
    for (; msg != NULL; msg = next_msg) {
        node_t **ptr = &maze_node(args->maze, msg->x, msg->y);
        node_t *node = *ptr;
        /* if NULL, the node is not opened */
        if (node == NULL) {
            /* allocate new node and modify maze.nodes. */
            node = node_init(alloc_node(pool), msg->x, msg->y);
            *ptr = node;
        }

        /* update if improved. */
        if (msg->gs < node->gs) {
            /* modify node. */
            node->parent = msg->parent;
            node->gs = msg->gs;
            node->fs = msg->gs + heuristic(node, get_goal(args->maze));
            if (node->heap_id != 0) {
                heap_update(heap, node);
                ++*msg_received;
            } else {
                heap_insert(heap, node);
            }
        } else {
            ++*msg_received;
        }
        next_msg = msg->next;
        free_msg(msg_queue, msg);
    }
}

void *hda_star_search(hda_argument_t *args) {
    mem_pool_t mem_pool;
    heap_t heap;
    size_t *msg_sent = &args->msg_sent[args->thread_id];
    size_t *msg_received = &args->msg_received[args->thread_id];
    hda_mq_t *msg_queue = &args->mqs[args->thread_id];

    /* init. */
    mem_pool_init(&mem_pool);
    heap_init(&heap);
    /* add start. */
    if (hash_distribute(args->thread_num, args->maze->start_x, args->maze->start_y) == args->thread_id) {
        node_t *node;
        /* initialize first node. */
        ++*msg_sent;
        node = node_init(alloc_node(&mem_pool), args->maze->start_x, args->maze->start_y);
        node->gs = 1;
        node->fs = 1 + heuristic(node, get_goal(args->maze));
        /* modify maze.nodes. */
        maze_node(args->maze, args->maze->start_x, args->maze->start_y) = node;
        /* insert first node. */
        heap_insert(&heap, node);
    }

    /* main loop. */
    while (!*args->finished) {
        if (heap.size > 1) {
            node_t *node, *other_node;
            /* if there are nodes in heap. */
            node = heap_extract(&heap);
            /* if the node is worse than currently found best path */
            if (node->gs >= args->return_value->min_len) {
                /* dump heap and add number of nodes to message received. */
                *msg_received += heap.size;
                heap.size = 1;
                continue;
            }
            /* if the node is opened in another list. */
            other_node = maze_node(args->other_maze, node->x, node->y);
            if (other_node != NULL) {
                /* update current best path. */
                update_return_value(
                        args->return_value_mutex, args->return_value,
                        node->gs + other_node->gs, node->x, node->y);
            } else {
                open_node(args, msg_queue, node, node->x + 1, node->y, msg_sent);
                open_node(args, msg_queue, node, node->x - 1, node->y, msg_sent);
                open_node(args, msg_queue, node, node->x, node->y + 1, msg_sent);
                open_node(args, msg_queue, node, node->x, node->y - 1, msg_sent);
            }
            /* message received add one */
            ++*msg_received;
        } else {
            if (detect_termination(args, msg_queue)) break;
        }

        receive_msg(args, msg_queue, &mem_pool, &heap, msg_received);
    }

    mem_pool_destroy(&mem_pool);
    heap_destroy(&heap);
    return NULL;
}

void *a_star_search(a_star_argument_t *args) {
    pthread_t *threads = NULL;
    hda_mq_t *message_queue = NULL;
    size_t *msg_sent = NULL;
    size_t *msg_received = NULL;
    hda_argument_t *args_for_threads = NULL;
    size_t i = 0;
    /* initialize and set up cleanups. */
    threads = malloc(args->thread_num * sizeof(pthread_t));
    message_queue = malloc(args->thread_num * sizeof(hda_mq_t));
    msg_sent = malloc(args->thread_num * sizeof(size_t));
    msg_received = malloc(args->thread_num * sizeof(size_t));
    args_for_threads = malloc(args->thread_num * sizeof(hda_argument_t));
    /* initialize thread each variables. */
    for (i = 0; i < args->thread_num; i++) {
        hda_mq_init(message_queue + i);
        msg_sent[i] = 0;
        msg_received[i] = 0;
        args_for_threads[i].file = args->file;
        args_for_threads[i].other_maze = args->other_maze;
        args_for_threads[i].maze = args->maze;
        args_for_threads[i].return_value_mutex = args->return_value_mutex;
        args_for_threads[i].return_value = args->return_value;
        args_for_threads[i].thread_num = args->thread_num;
        args_for_threads[i].thread_id = i;
        args_for_threads[i].mqs = message_queue;
        args_for_threads[i].msg_sent = msg_sent;
        args_for_threads[i].msg_received = msg_received;
        args_for_threads[i].finished = args->finished;
    }

    /* launch threads. */
    for (i = 0; i < args->thread_num; i++)
        assert(!pthread_create(threads + i, NULL, (void *(*)(void *)) hda_star_search, args_for_threads + i));
    /* join all the threads. */
    for (i = 0; i < args->thread_num; i++)
        assert(!pthread_join(threads[i], NULL));
    for (i = 0; i < args->thread_num; i++)
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
    maze_file_t *file = NULL;
    maze_t *start_maze = NULL, *goal_maze = NULL;
    pthread_mutex_t *return_value_mutex = NULL;
    a_star_return_t *return_value = NULL;
    size_t thread_num = get_nprocs();
    size_t *finished = NULL;
    a_star_argument_t *argument_start = NULL, *argument_goal = NULL;
    pthread_t from_start, from_goal;
    node_t *node = NULL;
    size_t count = 1;

    /* Must have given the source file name. */
    assert(argc == 2);
    /* Initializations. */
    file = maze_file_init(argv[1]);
    start_maze = maze_init(file->cols, file->rows, 1, 1, file->cols - 1, file->rows - 2);
    goal_maze = maze_init(file->cols, file->rows, file->cols - 2, file->rows - 2, 0, 1);
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
    argument_start->other_maze = goal_maze;
    argument_goal->other_maze = start_maze;
    argument_start->maze = start_maze;
    argument_goal->maze = goal_maze;
    argument_start->return_value_mutex = return_value_mutex;
    argument_goal->return_value_mutex = return_value_mutex;
    argument_start->return_value = return_value;
    argument_goal->return_value = return_value;
    argument_start->thread_num = thread_num / 2;
    argument_goal->thread_num = thread_num / 2;
    argument_start->finished = finished;
    argument_goal->finished = finished;

    /* create two threads. */
    assert(!pthread_create(&from_start, NULL, (void *(*)(void *)) a_star_search, argument_start));
    assert(!pthread_create(&from_goal, NULL, (void *(*)(void *)) a_star_search, argument_goal));
    /* join two threads thread. */
    assert(!pthread_join(from_start, NULL));
    assert(!pthread_join(from_goal, NULL));

/*    {
        size_t path_node = 0, open_node = 0;
        size_t x = 0, y = 0, rows = file->rows, cols = file->cols;
        for (y = 0; y < rows; y++) {
            for (x = 0; x < cols; x++) {
                if (maze_lines(file, x, y) != '#')
                    path_node += 1;
                if (maze_node(start_maze, x, y) != NULL || maze_node(goal_maze, x, y) != NULL)
                    open_node += 1;
            }
        }
        printf("%ld %ld\n", path_node, open_node);
    }*/

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
    printf("%ld\n", count);
    /* Free resources and return. */
    maze_file_destroy(file);
    maze_destroy(start_maze);
    maze_destroy(goal_maze);
    free(return_value_mutex);
    free(return_value);
    free(finished);
    free(argument_start);
    free(argument_goal);
    return 0;
}
