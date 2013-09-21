/*
 * pretty.c
 *
 *  Created on: May 1, 2011
 *      Author: infinoid
 */

#include <stdio.h>

#include "pretty.h"

int print(board_t *this) {
    myint_t x, y;
    printf("    0 1 2   3 4 5   6 7 8\n");
    printf("    ---------------------\n");
    for(y = 0; y < NUM_TOKENS; y++) {
        if(y && !(y % BOX_SIDE_LEN))
            printf("\n");
        printf("%d |", y);
        for(x = 0; x < NUM_TOKENS; x++) {
            myint_t v = this->results[y][x];
            if(x && !(x % BOX_SIDE_LEN))
                printf("  ");
            printf(" %c", v ? v+'0' : '.');
        }
        printf("\n");
    }
    return 0;
}
