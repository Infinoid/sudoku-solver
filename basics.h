/*
 * basics.h
 *
 *  Created on: May 1, 2011
 *      Author: infinoid
 */

#ifndef BASICS_H_
#define BASICS_H_

#include <stdio.h>
#include <inttypes.h>

#define BOX_CHOP 3
#define BOX_SIDE_LEN 3
#define NUM_TOKENS (BOX_CHOP*BOX_SIDE_LEN)

#define DEC(x) __sync_fetch_and_add(&x, -1)
#define CLZ(x) __builtin_clz(x)
#define CPOP(x) __builtin_popcount((x))
typedef int32_t myint_t;
typedef int32_t token_t;
typedef int32_t bitmask_t;

typedef struct link link_t;
struct link {
    link_t *next;
    myint_t address[2];
};

/* This data structure assumes 2 dimensions. */
typedef struct {
    token_t results[NUM_TOKENS][NUM_TOKENS];    /* [y][x] */
    bitmask_t per_unit[NUM_TOKENS][NUM_TOKENS]; /* [y][x] */
    link_t links[NUM_TOKENS][NUM_TOKENS];       /* [y][x] */
    link_t *pending; /* finished-queue to process */
    myint_t remaining;
} board_t;

#define debug(a, b...) do { printf("%s:%d: " a "\n", __FILE__, __LINE__, ##b); fflush(stderr); } while(0)
#define die(a, b...) do { debug("FATAL: " a, ##b); exit(1); } while(0)

#endif /* BASICS_H_ */
