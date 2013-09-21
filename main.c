/*
 * main.c
 *
 *  Created on: May 1, 2011
 *      Author: infinoid
 */

#include <stdio.h>

#include "basics.h"
#include "pretty.h"
#include "algorithm.h"
#include "generate.h"

int main(int argc, char **argv) {
    char *fn = "-";
    if(argc > 1)
        fn = argv[1];
    board_t *this = generate(fn);
    if(!this)
        return 1;
    annotate(this);
    while(this->remaining) {
        if(!chew(this)) {
            break;
        }
    }
    if(this->remaining)
        printf("Stopped making forward progress, %d spots remain.\n", this->remaining);
    else
        printf("Done.\n");
    print(this);
    return this->remaining;
}
