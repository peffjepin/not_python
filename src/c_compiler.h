#ifndef C_COMPILER_H
#define C_COMPILER_H

#include <stdio.h>

#include "lexer.h"

void compile_to_c(FILE* outfile, Lexer* lexer);

#endif
