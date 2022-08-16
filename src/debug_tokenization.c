#include <assert.h>
#include <stdio.h>

#include "lexer.h"

void print_token_type(TokenType type);
void print_operator_enum(Operator op);
void print_keyword(Keyword kw);

void
print_token(Token tok)
{
    printf("%u:%u\t", tok.line, tok.col);
    print_token_type(tok.type);
    if (tok.type == TOK_OPERATOR) {
        printf(": ");
        print_operator_enum(tok.operator);
    }
    else if (tok.type == TOK_KEYWORD) {
        printf(": ");
        print_keyword(tok.keyword);
    }
    else if (tok.type == TOK_NAME || tok.type == TOK_NUMBER || tok.type == TOK_STRING) {
        printf(": ");
        printf("%s", tok.string.buffer);
    }
    printf("\n");
}

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
    } while (tok.type != TOK_EOF);
    lex_close(&lex);
    return 0;
}

void
print_token_type(TokenType type)
{
    switch (type) {
        case NULL_TOKEN:
            printf("NULL_TOKEN");
            break;
        case TOK_EOF:
            printf("EOF");
            break;
        case TOK_COMMA:
            printf("COMMA");
            break;
        case TOK_KEYWORD:
            printf("KEYWORD");
            break;
        case TOK_COLON:
            printf("COLON");
            break;
        case TOK_STRING:
            printf("STRING");
            break;
        case TOK_NUMBER:
            printf("NUMBER");
            break;
        case TOK_OPERATOR:
            printf("OPERATOR");
            break;
        case TOK_NEWLINE:
            printf("NEWLINE");
            break;
        case TOK_BLOCK_BEGIN:
            printf("BLOCK_BEGIN");
            break;
        case TOK_BLOCK_END:
            printf("BLOCK_END");
            break;
        case TOK_OPEN_PARENS:
            printf("OPEN_PARENS");
            break;
        case TOK_CLOSE_PARENS:
            printf("CLOSE_PARENS");
            break;
        case TOK_OPEN_SQUARE:
            printf("OPEN_SQUARE");
            break;
        case TOK_CLOSE_SQUARE:
            printf("CLOSE_SQUARE");
            break;
        case TOK_OPEN_CURLY:
            printf("OPEN_CURLY");
            break;
        case TOK_CLOSE_CURLY:
            printf("CLOSE_CURLY");
            break;
        case TOK_NAME:
            printf("NAME");
            break;
        case TOK_DOT:
            printf("DOT");
            break;
    }
}

void
print_operator_enum(Operator op)
{
    switch (op) {
        case OP_PLUS:
            printf("PLUS");
            break;
        case OP_MINUS:
            printf("MINUS");
            break;
        case OP_MULT:
            printf("MULT");
            break;
        case OP_DIV:
            printf("DIV");
            break;
        case OP_MOD:
            printf("MOD");
            break;
        case OP_POW:
            printf("POW");
            break;
        case OP_FLOORDIV:
            printf("FLOORDIV");
            break;
        case OP_ASSIGNMENT:
            printf("ASSIGNMENT");
            break;
        case OP_PLUS_ASSIGNMENT:
            printf("PLUS_ASSIGNMENT");
            break;
        case OP_MINUS_ASSIGNMENT:
            printf("MINUS_ASSIGNMENT");
            break;
        case OP_MULT_ASSIGNMENT:
            printf("MULT_ASSIGNMENT");
            break;
        case OP_DIV_ASSIGNMENT:
            printf("DIV_ASSIGNMENT");
            break;
        case OP_MOD_ASSIGNMENT:
            printf("MOD_ASSIGNMENT");
            break;
        case OP_FLOORDIV_ASSIGNMENT:
            printf("FLOORDIV_ASSIGNMENT");
            break;
        case OP_POW_ASSIGNMENT:
            printf("POW_ASSIGNMENT");
            break;
        case OP_AND_ASSIGNMENT:
            printf("AND_ASSIGNMENT");
            break;
        case OP_OR_ASSIGNMENT:
            printf("OR_ASSIGNMENT");
            break;
        case OP_XOR_ASSIGNMENT:
            printf("XOR_ASSIGNMENT");
            break;
        case OP_RSHIFT_ASSIGNMENT:
            printf("RSHIFT_ASSIGNMENT");
            break;
        case OP_LSHIFT_ASSIGNMENT:
            printf("LSHIFT_ASSIGNMENT");
            break;
        case OP_EQUAL:
            printf("EQUAL");
            break;
        case OP_NOT_EQUAL:
            printf("NOT_EQUAL");
            break;
        case OP_GREATER:
            printf("GREATER");
            break;
        case OP_LESS:
            printf("LESS");
            break;
        case OP_GREATER_EQUAL:
            printf("GREATER_EQUAL");
            break;
        case OP_LESS_EQUAL:
            printf("LESS_EQUAL");
            break;
        case OP_BITWISE_AND:
            printf("BITWISE_AND");
            break;
        case OP_BITWISE_OR:
            printf("BITWISE_OR");
            break;
        case OP_BITWISE_XOR:
            printf("BITWISE_XOR");
            break;
        case OP_BITWISE_NOT:
            printf("BITWISE_NOT");
            break;
        case OP_LSHIFT:
            printf("LSHIFT");
            break;
        case OP_RSHIFT:
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
