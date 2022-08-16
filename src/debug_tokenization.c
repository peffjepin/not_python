#include <assert.h>
#include <stdio.h>

#include "debug_common.h"
#include "lexer.h"

int
main(int argc, char** argv)
{
    assert(argc == 2 && "useage './main [filename]'");
    Lexer lex = lex_open(argv[1]);
    scan_to_token_stream(&lex.scanner);
    Token tok;
    do {
        tok = token_stream_consume(&lex.scanner.ts);
        print_token(tok);
        printf("\n");
    } while (tok.type != TOK_EOF);
    lex_close(&lex);
    return 0;
}
