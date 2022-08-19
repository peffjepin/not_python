#include <assert.h>
#include <stdio.h>

#include "../src/lexer.h"
#include "debug_common.h"

int
main(int argc, char** argv)
{
    assert(argc == 2 && "useage './main [filename]'");
    Lexer lexer = lexer_open(argv[1]);
    lexer_tokenize(&lexer);
    debugger_use_arena(&lexer.arena);
    Token tok;
    do {
        for (size_t i = 0; i < lexer.arena.tokens_count; i++) {
            tok = arena_get_token(&lexer.arena, i);
            print_token(tok);
        }
    } while (tok.type != TOK_EOF);
    lexer_close(&lexer);
    return 0;
}
