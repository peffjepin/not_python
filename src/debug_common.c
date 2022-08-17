#include "debug_common.h"

#include <stdio.h>

void
print_statement(Statement stmt)
{
    for (size_t i = 0; i < stmt.length; i++) {
        Token tok = stmt.tokens[i];
        if (i > 0) printf(" ");
        if (tok.type == TOK_OPERATOR) {
            print_operator_enum(tok.operator);
        }
        else if (tok.type == TOK_KEYWORD) {
            print_keyword(tok.keyword);
        }
        else if (tok.type == TOK_NAME || tok.type == TOK_NUMBER || tok.type == TOK_STRING) {
            printf("%s", tok.string.buffer);
        }
        else
            print_token_type(tok.type);
    }
}

void
print_instruction(Instruction inst)
{
    switch (inst.type) {
        case NULL_INST:
            printf("NULL INST");
            break;
        case INST_EOF:
            printf("EOF");
            break;
        case INST_STMT:
            printf("STMT: ");
            print_statement(inst.stmt);
            break;
        case INST_FOR_LOOP:
            printf("FOR_LOOP: it=%s, iterable=", inst.for_loop.it.buffer);
            print_statement(inst.for_loop.iterable_stmt);
            break;
    }
    printf("\n");
}

void
print_token(Token tok)
{
    printf("%4u:%-4u", tok.line, tok.col);
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
        case OPERATOR_PLUS:
            printf("PLUS");
            break;
        case OPERATOR_MINUS:
            printf("MINUS");
            break;
        case OPERATOR_MULT:
            printf("MULT");
            break;
        case OPERATOR_DIV:
            printf("DIV");
            break;
        case OPERATOR_MOD:
            printf("MOD");
            break;
        case OPERATOR_POW:
            printf("POW");
            break;
        case OPERATOR_FLOORDIV:
            printf("FLOORDIV");
            break;
        case OPERATOR_ASSIGNMENT:
            printf("ASSIGNMENT");
            break;
        case OPERATOR_PLUS_ASSIGNMENT:
            printf("PLUS_ASSIGNMENT");
            break;
        case OPERATOR_MINUS_ASSIGNMENT:
            printf("MINUS_ASSIGNMENT");
            break;
        case OPERATOR_MULT_ASSIGNMENT:
            printf("MULT_ASSIGNMENT");
            break;
        case OPERATOR_DIV_ASSIGNMENT:
            printf("DIV_ASSIGNMENT");
            break;
        case OPERATOR_MOD_ASSIGNMENT:
            printf("MOD_ASSIGNMENT");
            break;
        case OPERATOR_FLOORDIV_ASSIGNMENT:
            printf("FLOORDIV_ASSIGNMENT");
            break;
        case OPERATOR_POW_ASSIGNMENT:
            printf("POW_ASSIGNMENT");
            break;
        case OPERATOR_AND_ASSIGNMENT:
            printf("AND_ASSIGNMENT");
            break;
        case OPERATOR_OR_ASSIGNMENT:
            printf("OR_ASSIGNMENT");
            break;
        case OPERATOR_XOR_ASSIGNMENT:
            printf("XOR_ASSIGNMENT");
            break;
        case OPERATOR_RSHIFT_ASSIGNMENT:
            printf("RSHIFT_ASSIGNMENT");
            break;
        case OPERATOR_LSHIFT_ASSIGNMENT:
            printf("LSHIFT_ASSIGNMENT");
            break;
        case OPERATOR_EQUAL:
            printf("EQUAL");
            break;
        case OPERATOR_NOT_EQUAL:
            printf("NOT_EQUAL");
            break;
        case OPERATOR_GREATER:
            printf("GREATER");
            break;
        case OPERATOR_LESS:
            printf("LESS");
            break;
        case OPERATOR_GREATER_EQUAL:
            printf("GREATER_EQUAL");
            break;
        case OPERATOR_LESS_EQUAL:
            printf("LESS_EQUAL");
            break;
        case OPERATOR_BITWISE_AND:
            printf("BITWISE_AND");
            break;
        case OPERATOR_BITWISE_OR:
            printf("BITWISE_OR");
            break;
        case OPERATOR_BITWISE_XOR:
            printf("BITWISE_XOR");
            break;
        case OPERATOR_BITWISE_NOT:
            printf("BITWISE_NOT");
            break;
        case OPERATOR_LSHIFT:
            printf("LSHIFT");
            break;
        case OPERATOR_RSHIFT:
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
