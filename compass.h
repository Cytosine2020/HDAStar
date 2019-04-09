/**
 * File: compass.h
 *
 *   Gives the only heuristic function you can use to estimate.
 *
 * Jose @ ShanghaiTech University
 */

#ifndef _COMPASS_H_
#define _COMPASS_H_

#include <stdlib.h>     /* abs */
#include "node.h"

/**
 * Heuristic function, using simply manhattan distance between N1 and N2.
 *   Returns the distance (i.e. h(n1, n2)).
 */
int heuristic(node_t *n1, node_t *n2) {
    return abs(n1->x - n2->x) + abs(n1->y - n2->y);
}

#endif
