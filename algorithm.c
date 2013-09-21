/*
 * algorithm.c
 *
 *  Created on: May 1, 2011
 *      Author: infinoid
 */

#include <stdio.h>
#include <stdlib.h>

#include "algorithm.h"
#include "pretty.h"

void mask_box(board_t *this, myint_t mask, myint_t x, myint_t y) {
    myint_t new, prev = __sync_fetch_and_and(&this->per_unit[y][x], mask);
    token_t found;
    if(prev == (prev & mask))
        /* No change, don't bother. */
        return;
    new = prev & mask;
    if(!new) {
        /* This indicates a logic error, or an unsolvable puzzle. */
        print(this);
        die("Either a logic error occurred, or this puzzle is unsolvable.\n"
            "(Attempt to apply mask 0x%x to grid unit (%d,%d) (prev 0x%x) resulted in 0.)\n", mask, x, y, prev);
    }
    if(!(new & (new-1))) {
        /* Found another solution.  Add it to the queue. */
        DEC(this->remaining);
        found = (sizeof(new)<<3) - CLZ(new);
        this->results[y][x] = found;
        this->links[y][x].next = this->pending;
        this->pending = &this->links[y][x];
    }
}

void mark_done(board_t *this, myint_t x, myint_t y);
void mark_done(board_t *this, myint_t x, myint_t y) {
    myint_t i, j, boxx, boxy, mask;
    mask = ~(1<<(this->results[y][x]-1));
    for(i = 0; i < NUM_TOKENS; i++) {
        /* Mark the row */
        if(i != y)
            mask_box(this, mask, x, i);
        /* Mark the column */
        if(i != x)
            mask_box(this, mask, i, y);
    }
    boxx = x - (x % BOX_SIDE_LEN);
    boxy = y - (y % BOX_SIDE_LEN);
    /* Mark the box */
    for(i = boxx; i < boxx + BOX_SIDE_LEN; i++) for(j = boxy; j < boxy + BOX_SIDE_LEN; j++)
        if(i != x || j != y)
            mask_box(this, mask, i, j);
}

int mark_pending(board_t *board) {
    myint_t count = 0;
    link_t *this = board->pending;
    while(this) {
        count++;
        board->pending = this->next;
        mark_done(board, this->address[0], this->address[1]);
        this = board->pending;
    }
    if(count)
        fprintf(stderr, "mark_pending: handled %d pending completions.\n", count);
    return count;
}

/* Given a board_t that's memsetted to 0, and then results[][] populated
 * with preexisting data, initialize the rest of board_t.
 */
int annotate(board_t *this) {
    myint_t x, y, count = NUM_TOKENS*NUM_TOKENS;
    /* First pass, initialize masks and generate count. */
    for(y = 0; y < NUM_TOKENS; y++) for(x = 0; x < NUM_TOKENS; x++) {
        myint_t v = this->results[y][x];
        if(v) {
            count--;
            this->per_unit[y][x] = 1 << (v - 1);
        } else {
            this->per_unit[y][x] = (1 << NUM_TOKENS) - 1;
        }
        this->links[y][x].address[0] = x;
        this->links[y][x].address[1] = y;
    }
    this->remaining = count;
    /* Second pass, update empty-cell masks with full-cell data. */
    for(y = 0; y < NUM_TOKENS; y++)
        for(x = 0; x < NUM_TOKENS; x++)
            if(this->results[y][x]) {
                mark_done(this, x, y);
            }
    return count;
}

static inline myint_t count_unresolved_flags(board_t *this, myint_t bx, myint_t by, myint_t sx, myint_t sy, myint_t m) {
    myint_t x, y, rv = 0;
    myint_t flag = 1<<m;
    for(x = bx; x < bx+sx; x++) for(y = by; y < by+sy; y++) {
        if(this->results[y][x] == m+1)
            return -1;
        if(this->per_unit[y][x] & flag)
            rv++;
    }
    return rv;
}

static inline myint_t last_one_standing_in_line(board_t *this) {
    myint_t n, i, flag, rv = 0;
    for(i = 0; i < NUM_TOKENS; i++) {
        flag = 1<<i;
        for(n = 0; n < NUM_TOKENS; n++) {
            if(count_unresolved_flags(this, 0, n, NUM_TOKENS, 1, i) == 1) {
                /* There's only one possible "i" for this row. */
                myint_t x;
                printf("Only one %d in row %d.\n", i+1, n);
                for(x = 0; x < NUM_TOKENS; x++) {
                    if(this->per_unit[n][x] & flag) {
                        mask_box(this, flag, x, n);
                        rv++;
                        break;
                    }
                }
            }
            if(count_unresolved_flags(this, n, 0, 1, NUM_TOKENS, i) == 1) {
                /* There's only one possible "i" for this column. */
                myint_t y;
                printf("Only one %d in column %d.\n", i+1, n);
                for(y = 0; y < NUM_TOKENS; y++) {
                    if(this->per_unit[y][n] & flag) {
                        mask_box(this, flag, n, y);
                        rv++;
                        break;
                    }
                }
            }
        }
    }
    return rv;
}

static inline myint_t last_one_standing_in_box(board_t *this) {
    myint_t i, flag, bx, by, rv = 0;
    for(i = 0; i < NUM_TOKENS; i++) {
        flag = 1<<i;
        for(bx = 0; bx < NUM_TOKENS; bx += BOX_SIDE_LEN) for(by = 0; by < NUM_TOKENS; by += BOX_SIDE_LEN) {
            if(count_unresolved_flags(this, bx, by, BOX_SIDE_LEN, BOX_SIDE_LEN, i) == 1) {
                /* There's only one possible "i" for this box. */
                myint_t x, y;
                printf("Only one %d in box at (%d,%d).\n", i+1, bx, by);
                for(x = bx; x < bx + BOX_SIDE_LEN; x++) for(y = by; y < by + BOX_SIDE_LEN; y++) {
                    if(this->per_unit[y][x] & flag) {
                        mask_box(this, flag, x, y);
                        rv++;
                        break;
                    }
                }
            }
        }
    }
    return rv;
}

/* Note: the following searches are only done when the algorithm is
 * somewhat desperate, as matches don't necessarily ensure forward
 * progress.
 */
static inline myint_t box_items_form_a_line(board_t *this) {
    myint_t i, l, n, boxcount, bx, by, flag, rv = 0;
    for(i = 0; i < NUM_TOKENS; i++) {
        flag = 1<<i;
        for(bx = 0; bx < NUM_TOKENS; bx += BOX_SIDE_LEN) for(by = 0; by < NUM_TOKENS; by += BOX_SIDE_LEN) {
            boxcount = count_unresolved_flags(this, bx, by, BOX_SIDE_LEN, BOX_SIDE_LEN, i);
            if(boxcount < 0)
                continue;
            for(l = 0; l < BOX_SIDE_LEN; l++) {
                if(boxcount == count_unresolved_flags(this, bx, by+l, BOX_SIDE_LEN, 1, i)) {
                    for(n = 0; n < NUM_TOKENS; n++) {
                        if(n < bx || n >= bx + BOX_SIDE_LEN) {
                            if(!this->results[by+l][n] && this->per_unit[by+l][n] & flag) {
                                printf("The %d's in box at (%d,%d) form a horizontal line, which rules out (%d,%d).\n",
                                       i+1, bx, by, n, by+l);
                                rv++;
                                mask_box(this, ~flag, n, by+l);
                            }
                        }
                    }
                }
                if(boxcount == count_unresolved_flags(this, bx+l, by, 1, BOX_SIDE_LEN, i)) {
                    for(n = 0; n < NUM_TOKENS; n++) {
                        if(n < by || n >= by + BOX_SIDE_LEN) {
                            if(!this->results[n][bx+l] && this->per_unit[n][bx+l] & flag) {
                                printf("The %d's in box at (%d,%d) form a vertical line, which rules out (%d,%d).\n",
                                       i+1, bx, by, bx+l, n);
                                rv++;
                                mask_box(this, ~flag, bx+l, n);
                            }
                        }
                    }
                }
            }
        }
    }
    return rv;
}

static inline myint_t line_items_are_in_a_box(board_t *this) {
    myint_t i, j, k, l, bx, by, flag, rv = 0;
    for(l = 0; l < NUM_TOKENS; l++) {
        flag = 1<<l;
        for(i = 0; i < NUM_TOKENS; i++) {
            int row_count = 0, col_count = 0;
            for(j = 0; j < NUM_TOKENS; j++) {
                if(this->per_unit[i][j] & flag)
                    row_count++;
                if(this->per_unit[j][i] & flag)
                    col_count++;
            }
            for(k = 0; k < NUM_TOKENS; k += BOX_SIDE_LEN) {
                int box_row_count = 0, box_col_count = 0;
                for(j = 0; j < BOX_SIDE_LEN; j++) {
                    if(this->per_unit[i][j+k] & flag)
                        box_row_count++;
                    if(this->per_unit[j+k][i] & flag)
                        box_col_count++;
                }
                if(row_count == box_row_count) {
                    for(by = i - (i % BOX_SIDE_LEN); by < i - (i % BOX_SIDE_LEN) + BOX_SIDE_LEN; by++) {
                        if(by == i)
                            continue;
                        for(bx = k; bx < k + BOX_SIDE_LEN; bx++) {
                            if(this->per_unit[by][bx] & flag) {
                                printf("The %d's in line (%d,*) are in a box, which rules out (%d,%d).\n", l+1, i, bx, by);
                                rv++;
                                mask_box(this, ~flag, bx, by);
                            }
                        }
                    }
                }
                if(col_count == box_col_count) {
                    for(bx = i - (i % BOX_SIDE_LEN); bx < i - (i % BOX_SIDE_LEN) + BOX_SIDE_LEN; bx++) {
                        if(bx == i)
                            continue;
                        for(by = k; by < k + BOX_SIDE_LEN; by++) {
                            if(this->per_unit[by][bx] & flag) {
                                printf("The %d's in column (*,%d) are in a box, which rules out (%d,%d).\n", l+1, i, bx, by);
                                rv++;
                                mask_box(this, ~flag, bx, by);
                            }
                        }
                    }
                }
            }
        }
    }
    return rv;
}

static inline myint_t inductive_exclusion_2(board_t *this, myint_t bx, myint_t by, myint_t sx, myint_t sy) {
    /* Given a subset of the space where exclusion is enforced, find
     * matched tuples of possibilities for which inductive closure excludes
     * those possibilities from existing anywhere else in the set.
     *
     * In other words, if two cells of a box both have 2 and 6 as options,
     * and they can't be anything else, then some third box which has 2,
     * 6 and 9 as options can't be a 2 or 6 after all. */
    myint_t i, rv = 0;
    for(i = 0; i < NUM_TOKENS - 1; i++) {
        myint_t j, ix, iy, mask;
        ix = i % sx + bx;
        iy = i / sx + by;
        mask = this->per_unit[iy][ix];
        if(CPOP(mask) != 2)
            continue;
        for(j = i + 1; j < NUM_TOKENS; j++) {
            myint_t k, jx, jy;
            jx = j % sx + bx;
            jy = j / sx + by;
            if(this->per_unit[jy][jx] != mask)
                continue;

            for(k = 0; k < NUM_TOKENS; k++) {
                myint_t kx, ky;
                kx = k % sx + bx;
                ky = k / sx + by;
                if(k == i || k == j)
                    continue;
                if(!(this->per_unit[ky][kx] & mask))
                    continue;
                printf("Found pair [%d,%d] and [%d,%d] with equal sets of 2; eliminating those from [%d,%d].\n", ix, iy, jx, jy, kx, ky);
                mask_box(this, ~mask, kx, ky);
                rv++;
            }
        }
    }
    return rv;
}

static inline myint_t inductive_exclusion_3(board_t *this, myint_t bx, myint_t by, myint_t sx, myint_t sy) {
    /* Same thing as above, but looking for 3 sets of 3 possibilities.
     *
     * In other words, if three cells of a box all have 2, 6 and 8 as options,
     * and they can't be anything else, then some fourth box which has 2,
     * 6, 8 and 9 as options can't be a 2 or 6 or 8 after all. */
    myint_t i, rv = 0;
    for(i = 0; i < NUM_TOKENS - 2; i++) {
        myint_t j, ix, iy, mask;
        ix = i % sx + bx;
        iy = i / sx + by;
        mask = this->per_unit[iy][ix];
        if(CPOP(mask) != 3)
            continue;
        for(j = i + 1; j < NUM_TOKENS-1; j++) {
            myint_t k, jx, jy;
            jx = j % sx + bx;
            jy = j / sx + by;
            if(this->per_unit[jy][jx] != mask)
                continue;
            for(k = j + 1; k < NUM_TOKENS; k++) {
                myint_t l, kx, ky;
                kx = k % sx + bx;
                ky = k / sx + by;
                if(this->per_unit[ky][kx] != mask)
                    continue;
                for(l = 0; l < NUM_TOKENS; l++) {
                    myint_t lx, ly;
                    lx = l % sx + bx;
                    ly = l / sx + by;
                    if(l == i || l == j || l == k)
                        continue;
                    if(!(this->per_unit[ly][lx] & mask))
                        continue;
                    printf("Found triple [%d,%d], [%d,%d] and [%d,%d] with equal sets of 3; eliminating those from [%d,%d].\n", ix, iy, jx, jy, kx, ky, lx, ly);
                    mask_box(this, ~mask, lx, ly);
                    rv++;
                }
            }
        }
    }
    return rv;
}

static inline myint_t inductive_exclusion(board_t *this) {
    myint_t i, j, rv = 0;
    for(i = 0; i < NUM_TOKENS; i += BOX_SIDE_LEN) for(j = 0; j < NUM_TOKENS; j += BOX_SIDE_LEN)
        rv += inductive_exclusion_2(this, i, j, BOX_SIDE_LEN, BOX_SIDE_LEN);
    for(i = 0; i < NUM_TOKENS; i++) {
        rv += inductive_exclusion_2(this, i, 0, 1, NUM_TOKENS);
        rv += inductive_exclusion_2(this, 0, i, NUM_TOKENS, 1);
    }
    if(rv)
        return rv;
    for(i = 0; i < NUM_TOKENS; i += BOX_SIDE_LEN) for(j = 0; j < NUM_TOKENS; j += BOX_SIDE_LEN) 
        rv += inductive_exclusion_3(this, i, j, BOX_SIDE_LEN, BOX_SIDE_LEN);
    for(i = 0; i < NUM_TOKENS; i++) {
        rv += inductive_exclusion_3(this, i, 0, 1, NUM_TOKENS);
        rv += inductive_exclusion_3(this, 0, i, NUM_TOKENS, 1);
    }
    return rv;
}

int chew(board_t *this) {
    myint_t count = 0, rv;
    while((rv = mark_pending(this))) {
        count += rv;
    }
    if(!this->remaining)
        return rv;
    /* last one in box standing */
    rv += last_one_standing_in_box(this);
    /* last one in line standing */
    rv += last_one_standing_in_line(this);
    if(rv)
        return rv;
    /* items in box form a line */
    rv += box_items_form_a_line(this);
    /* items in line are within a box */
    rv += line_items_are_in_a_box(this);
    /* inductive exclusion */
    rv += inductive_exclusion(this);

    return rv;
}
