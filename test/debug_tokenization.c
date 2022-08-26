#include <assert.h>
#include <stdio.h>

#include "../src/lexer.h"
#include "debug_common.h"

#if DEBUG
int
main(int argc, char** argv)
{
    assert(argc == 2 && "useage './main [filename]'");
    Lexer lexer = lex_file(argv[1]);
    debugger_use_arena(&lexer.arena);
    Token tok;
    do {
        for (size_t i = 0; i < lexer.arena.tokens_count; i++) {
            tok = lexer.arena.tokens[i];
            print_token(tok);
        }
    } while (tok.type != TOK_EOF);
    lexer_free(&lexer);
    return 0;
}
#endif
