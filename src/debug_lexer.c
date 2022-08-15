#include <assert.h>
#include <stdio.h>

#include "lexer.h"

void
print_token_type(TokenType type)
{
    switch (type) {
        case NULL_TOKEN:
            printf("NULL_TOKEN");
            break;
        case KEYWORD:
            printf("KEYWORD");
            break;
        case COLON:
            printf("COLON");
            break;
        case STRING:
            printf("STRING");
            break;
        case NUMBER:
            printf("NUMBER");
            break;
        case OPERATOR:
            printf("OPERATOR");
            break;
        case NEWLINE:
            printf("NEWLINE");
            break;
        case BLOCK_BEGIN:
            printf("BLOCK_BEGIN");
            break;
        case BLOCK_END:
            printf("BLOCK_END");
            break;
        case OPEN_PARENS:
            printf("OPEN_PARENS");
            break;
        case CLOSE_PARENS:
            printf("CLOSE_PARENS");
            break;
        case OPEN_SQUARE:
            printf("OPEN_SQUARE");
            break;
        case CLOSE_SQUARE:
            printf("CLOSE_SQUARE");
            break;
        case OPEN_CURLY:
            printf("OPEN_CURLY");
            break;
        case CLOSE_CURLY:
            printf("CLOSE_CURLY");
            break;
        case NAME:
            printf("NAME");
            break;
        case DOT:
            printf("DOT");
            break;
    }
}

void
print_operator_enum(Operator op)
{
    switch (op) {
        case PLUS:
            printf("PLUS");
            break;
        case MINUS:
            printf("MINUS");
            break;
        case MULT:
            printf("MULT");
            break;
        case DIV:
            printf("DIV");
            break;
        case MOD:
            printf("MOD");
            break;
        case POW:
            printf("POW");
            break;
        case FLOORDIV:
            printf("FLOORDIV");
            break;
        case ASSIGNMENT:
            printf("ASSIGNMENT");
            break;
        case PLUS_ASSIGNMENT:
            printf("PLUS_ASSIGNMENT");
            break;
        case MINUS_ASSIGNMENT:
            printf("MINUS_ASSIGNMENT");
            break;
        case MULT_ASSIGNMENT:
            printf("MULT_ASSIGNMENT");
            break;
        case DIV_ASSIGNMENT:
            printf("DIV_ASSIGNMENT");
            break;
        case MOD_ASSIGNMENT:
            printf("MOD_ASSIGNMENT");
            break;
        case FLOORDIV_ASSIGNMENT:
            printf("FLOORDIV_ASSIGNMENT");
            break;
        case POW_ASSIGNMENT:
            printf("POW_ASSIGNMENT");
            break;
        case AND_ASSIGNMENT:
            printf("AND_ASSIGNMENT");
            break;
        case OR_ASSIGNMENT:
            printf("OR_ASSIGNMENT");
            break;
        case XOR_ASSIGNMENT:
            printf("XOR_ASSIGNMENT");
            break;
        case RSHIFT_ASSIGNMENT:
            printf("RSHIFT_ASSIGNMENT");
            break;
        case LSHIFT_ASSIGNMENT:
            printf("LSHIFT_ASSIGNMENT");
            break;
        case EQUAL:
            printf("EQUAL");
            break;
        case NOT_EQUAL:
            printf("NOT_EQUAL");
            break;
        case GREATER:
            printf("GREATER");
            break;
        case LESS:
            printf("LESS");
            break;
        case GREATER_EQUAL:
            printf("GREATER_EQUAL");
            break;
        case LESS_EQUAL:
            printf("LESS_EQUAL");
            break;
        case BITWISE_AND:
            printf("BITWISE_AND");
            break;
        case BITWISE_OR:
            printf("BITWISE_OR");
            break;
        case BITWISE_XOR:
            printf("BITWISE_XOR");
            break;
        case BITWISE_NOT:
            printf("BITWISE_NOT");
            break;
        case LSHIFT:
            printf("LSHIFT");
            break;
        case RSHIFT:
            printf("RSHIFT");
            break;
    }
}

void
print_keyword(Keyword kw)
{
    switch (kw) {
        case NOT_A_KEYWORD:
            printf("NOT_A_KEYWORD");
            break;
        case KW_AND:
            printf("KW_AND");
            break;
        case KW_AS:
            printf("KW_AS");
            break;
        case KW_ASSERT:
            printf("KW_ASSERT");
            break;
        case KW_BREAK:
            printf("KW_BREAK");
            break;
        case KW_CLASS:
            printf("KW_CLASS");
            break;
        case KW_CONTINUE:
            printf("KW_CONTINUE");
            break;
        case KW_DEF:
            printf("KW_DEF");
            break;
        case KW_DEL:
            printf("KW_DEL");
            break;
        case KW_ELIF:
            printf("KW_ELIF");
            break;
        case KW_ELSE:
            printf("KW_ELSE");
            break;
        case KW_EXCEPT:
            printf("KW_EXCEPT");
            break;
        case KW_FINALLY:
            printf("KW_FINALLY");
            break;
        case KW_FOR:
            printf("KW_FOR");
            break;
        case KW_FROM:
            printf("KW_FROM");
            break;
        case KW_GLOBAL:
            printf("KW_GLOBAL");
            break;
        case KW_IF:
            printf("KW_IF");
            break;
        case KW_IMPORT:
            printf("KW_IMPORT");
            break;
        case KW_IN:
            printf("KW_IN");
            break;
        case KW_IS:
            printf("KW_IS");
            break;
        case KW_LAMBDA:
            printf("KW_LAMBDA");
            break;
        case KW_NONLOCAL:
            printf("KW_NONLOCAL");
            break;
        case KW_NOT:
            printf("KW_NOT");
            break;
        case KW_OR:
            printf("KW_OR");
            break;
        case KW_PASS:
            printf("KW_PASS");
            break;
        case KW_RAISE:
            printf("KW_RAISE");
            break;
        case KW_RETURN:
            printf("KW_RETURN");
            break;
        case KW_TRY:
            printf("KW_TRY");
            break;
        case KW_WHILE:
            printf("KW_WHILE");
            break;
        case KW_WITH:
            printf("KW_WITH");
            break;
        case KW_YIELD:
            printf("KW_YIELD");
            break;
    }
}

void
print_token(Token tok)
{
    printf("%u:%u\t", tok.line, tok.col);
    print_token_type(tok.type);
    if (tok.type == OPERATOR) {
        printf(": ");
        print_operator_enum(tok.operator);
    }
    else if (tok.type == KEYWORD) {
        printf(": ");
        print_keyword(tok.keyword);
    }
    else if (tok.type == NAME) {
        printf(": ");
        printf("%s", tok.value.buffer);
    }
    printf("\n");
}

int
main(int argc, char** argv)
{
    assert(argc == 2 && "useage './main [filename]'");
    FILE* testfile = fopen(argv[1], "r");
    if (!testfile) {
        fprintf(stderr, "failed to open file: %s\n", argv[1]);
        exit(1);
    }
    Lexer lex = {.srcfile = testfile};
    Token tok = next_token(&lex);
    do {
        print_token(tok);
        tok = next_token(&lex);
    } while (tok.type != NULL_TOKEN);
    fclose(testfile);
    return 0;
}
