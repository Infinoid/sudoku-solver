/*
 * generate.c
 *
 *  Created on: May 1, 2011
 *      Author: infinoid
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "generate.h"

board_t *generate(char *fn) {
    FILE *f = fopen(fn, "r");
    char text[50];
    int line = 0, col;
    if(!f)
         die("Cannot open input file \"%s\"\n", fn);
    board_t *rv = malloc(sizeof(board_t));
    if(!rv)
        return rv;
    memset(rv, 0, sizeof(board_t));
    while(fgets(text, sizeof(text), f)) {
        if(line >= NUM_TOKENS)
            die("Too many lines in %s.\n", fn);
        text[sizeof(text)-1] = 0;
        if(strlen(text) != NUM_TOKENS + 1)
            die("line %d of %s has too many characters.\n", line+1, fn);
        for(col = 0; col < NUM_TOKENS; col++) {
            if(text[col] >= '0' && text[col] <= '9')
                rv->results[line][col] = text[col] - '0';
            else if(text[col] != '.')
                die("Invalid character %c in %s.\n", text[col], fn);
        }
        line++;
    }
    return rv;
}
