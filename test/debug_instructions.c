#include <assert.h>
#include <stdio.h>

#include "../src/lexer.h"
#include "debug_common.h"

int
main(int argc, char** argv)
{
    assert(argc == 2 && "useage './debug_inst [filename]'");
    Lexer lexer = lexer_open(argv[1]);
    lexer_tokenize(&lexer);
    Instruction* instructions = lexer_parse_instructions(&lexer);
    Instruction inst;
    do {
        inst = *instructions++;
        print_instruction(inst);
    } while (inst.type != INST_EOF);
    lexer_close(&lexer);
    return 0;
}
