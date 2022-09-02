#define DEBUG 1
#include <assert.h>
#include <stdio.h>

#include "../src/lexer.h"
#include "debug_common.h"

int
main(int argc, char** argv)
{
    assert(argc == 2 && "useage './debug_scopes [filename]'");
    Lexer lexer = lex_file(argv[1]);
    print_scopes(&lexer);
    lexer_free(&lexer);
    return 0;
}
