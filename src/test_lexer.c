#include <stdio.h>

#include "lexer.h"

int
main()
{
    FILE* testfile = fopen("testfile.py", "r");
    if (!testfile) {
        fprintf(stderr, "failed to open file\n");
        exit(1);
    }
    Lexer lex = {.srcfile = testfile};
    Token tok = next_token(&lex);
    do {
        switch (tok.type) {
            case NULL_TOKEN:
                printf("NULL_TOKEN, %u:%u\n", tok.line, tok.col);
                break;
            case KEYWORD:
                printf("KEYWORD, %u:%u\n", tok.line, tok.col);
                break;
            case COLON:
                printf("COLON, %u:%u\n", tok.line, tok.col);
                break;
            case STRING:
                printf("STRING, %u:%u\n", tok.line, tok.col);
                break;
            case NUMBER:
                printf("NUMBER, %u:%u\n", tok.line, tok.col);
                break;
            case OPERATOR:
                printf("OPERATOR, %u:%u\n", tok.line, tok.col);
                break;
            case NEWLINE:
                printf("NEWLINE, %u:%u\n", tok.line, tok.col);
                break;
            case BLOCK_BEGIN:
                printf("BLOCK_BEGIN, %u:%u\n", tok.line, tok.col);
                break;
            case BLOCK_END:
                printf("BLOCK_END, %u:%u\n", tok.line, tok.col);
                break;
            case OPEN_PARENS:
                printf("OPEN_PARENS, %u:%u\n", tok.line, tok.col);
                break;
            case CLOSE_PARENS:
                printf("CLOSE_PARENS, %u:%u\n", tok.line, tok.col);
                break;
            case OPEN_SQUARE:
                printf("OPEN_SQUARE, %u:%u\n", tok.line, tok.col);
                break;
            case CLOSE_SQUARE:
                printf("CLOSE_SQUARE, %u:%u\n", tok.line, tok.col);
                break;
            case OPEN_CURLY:
                printf("OPEN_CURLY, %u:%u\n", tok.line, tok.col);
                break;
            case CLOSE_CURLY:
                printf("CLOSE_CURLY, %u:%u\n", tok.line, tok.col);
                break;
            case NAME:
                printf("NAME, %u:%u\n", tok.line, tok.col);
                break;
            case DOT:
                printf("DOT, %u:%u\n", tok.line, tok.col);
                break;
        }
        tok = next_token(&lex);
    } while (tok.type != NULL_TOKEN);
    fclose(testfile);
    return 0;
}
