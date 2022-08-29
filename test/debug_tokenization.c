#define DEBUG 1

#include <assert.h>
#include <stdio.h>

#include "../src/lexer.h"
#include "debug_common.h"

int
main(int argc, char** argv)
{
    assert(argc == 2 && "useage './main [filename]'");
    size_t count;
    Token* tokens = tokenize_file(argv[1], &count);
    for (size_t i = 0; i < count; i++) {
        print_token(tokens[i]);
    }
    free(tokens);
    return 0;
}
