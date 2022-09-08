#if DEBUG

#include <stddef.h>
#include <stdlib.h>

#include "debug_common.h"
#include "c_compiler.h"
#include "lexer.h"
#include "tokens.h"

void
debug_tokens_main(char* filepath)
{
    size_t count;
    Token* tokens = tokenize_file(filepath, &count);
    for (size_t i = 0; i < count; i++) {
        print_token(tokens[i]);
    }
    free(tokens);
}

void
debug_scopes_main(char* filepath)
{
    Lexer lexer = lex_file(filepath);
    print_scopes(&lexer);
    lexer_free(&lexer);
}

void
debug_statements_main(char* filepath)
{
    Lexer lexer = lex_file(filepath);
    for (size_t i = 0; i < lexer.n_statements; i++) {
        print_statement(lexer.statements[i], 0);
    }
    lexer_free(&lexer);
}

void
debug_c_compiler_main(char* filepath)
{
    Lexer lexer = lex_file(filepath);
    compile_to_c(stdout, &lexer);
    lexer_free(&lexer);
}

#endif
