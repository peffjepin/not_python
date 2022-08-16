#include <assert.h>
#include <stdio.h>

#include "debug_common.h"
#include "lexer.h"

int
main(int argc, char** argv)
{
    assert(argc == 2 && "useage './debug_inst [filename]'");
    Lexer lex = lex_open(argv[1]);
    scan_to_token_stream(&lex.scanner);
    Instruction inst;
    do {
        inst = next_instruction(&lex);
        print_instruction(inst);
    } while (inst.type != INST_EOF);
    lex_close(&lex);
    return 0;
}
