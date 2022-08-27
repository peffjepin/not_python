#include <assert.h>
#include <stdio.h>

#include "../src/lexer.h"
#include "debug_common.h"

int
main(int argc, char** argv)
{
    assert(argc == 2 && "useage './debug_inst [filename]'");
    Lexer lexer = lex_file(argv[1]);
    debugger_use_arena(&lexer.arena);
    for (size_t i = 0; i < lexer.n_statements; i++) {
        print_statement(lexer.statements + i);
    }
    lexer_free(&lexer);
    return 0;
}
