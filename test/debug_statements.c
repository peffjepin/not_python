#include <assert.h>
#include <stdio.h>

#include "../src/lexer.h"
#include "debug_common.h"

int
main(int argc, char** argv)
{
    assert(argc == 2 && "useage './debug_inst [filename]'");
    Lexer lexer = lexer_open(argv[1]);
    debugger_use_arena(&lexer.arena);
    lexer_tokenize(&lexer);
    lexer_parse_instructions(&lexer);
    Statement stmt;
    do {
        for (size_t i = 0; i < lexer.arena.statements_count; i++) {
            stmt = arena_get_statement(&lexer.arena, i);
            print_statement(stmt);
        }
    } while (stmt.kind != STMT_EOF);
    lexer_close(&lexer);
    return 0;
}
