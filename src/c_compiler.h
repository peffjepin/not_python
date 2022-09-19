#ifndef C_COMPILER_H
#define C_COMPILER_H

#include <stdio.h>

#include "lexer.h"

typedef enum { LIB_MATH, LIB_COUNT } Libraries;

typedef struct {
    bool libs[LIB_COUNT];
} Requirements;

Requirements compile_to_c(FILE* outfile, Lexer* lexer);

#endif
