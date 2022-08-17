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
    Token tok;
    do {
        tok = token_stream_consume(&lexer.ts);
        print_token(tok);
    } while (tok.type != TOK_EOF);
    lexer_close(&lexer);
    return 0;
}
