#include <assert.h>
#include <stdio.h>

#include "../src/c_compiler.h"
#include "../src/lexer.h"

int
main(int argc, char** argv)
{
    assert(argc == 2 && "useage './debug_c_compiler [filename]'");
    Lexer lexer = lex_file(argv[1]);
    compile_to_c(stdout, &lexer);
    lexer_free(&lexer);
    return 0;
}
