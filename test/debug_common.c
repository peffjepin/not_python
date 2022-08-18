#include "debug_common.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define STRING_BUFFER_CAPACITY 1024

typedef struct {
    char data[STRING_BUFFER_CAPACITY];
    size_t length;
} StringBuffer;

static StringBuffer render_expr(Expression expr);
static const char* operator_enum_to_cstr(Operator op);

static void
str_append_char(StringBuffer* str, char c)
{
    str->data[str->length++] = c;
}

static void
str_concat(StringBuffer* str1, StringBuffer* str2)
{
    assert(str1->length + str2->length <= STRING_BUFFER_CAPACITY);
    memcpy(str1->data + str1->length, str2->data, str2->length);
    str1->length += str2->length;
}

static void
str_concat_cstr(StringBuffer* str1, char* cstr)
{
    size_t length = strlen(cstr);
    assert(str1->length + length <= STRING_BUFFER_CAPACITY);
    memcpy(str1->data + str1->length, cstr, length);
    str1->length += length;
}

static void
render_operand(StringBuffer* str, Operand op)
{
    switch (op.kind) {
        case OPERAND_CONSTANT:
            str_concat_cstr(str, op.constant.value.buffer);
            break;
        case OPERAND_STORAGE:
            str_concat_cstr(str, op.storage.identifier.buffer);
            break;
        case OPERAND_EXPRESSION: {
            StringBuffer rendered_expr = render_expr(*op.expression);
            str_concat(str, &rendered_expr);
            break;
        }
        default:
            assert(0 && "not implemented");
    }
}

static StringBuffer string_mem[EXPRESSION_CAPACITY] = {0};

static StringBuffer
render_expr(Expression expr)
{
    StringBuffer* already_rendered_chunks[EXPRESSION_CAPACITY] = {0};

    if (expr.n_operations == 0) {
        StringBuffer buf = {0};
        render_operand(&buf, expr.operands[0]);
        return buf;
    }

    for (size_t i = 0; i < expr.n_operations; i++) {
        Operation operation = expr.operations[i];
        StringBuffer this_operation_rendered = {0};

        str_append_char(&this_operation_rendered, '(');
        StringBuffer* left_ref = already_rendered_chunks[operation.left];
        if (left_ref)
            str_concat(&this_operation_rendered, left_ref);
        else
            render_operand(&this_operation_rendered, expr.operands[operation.left]);
        str_append_char(&this_operation_rendered, ' ');
        str_concat_cstr(
            &this_operation_rendered, (char*)operator_enum_to_cstr(operation.op_type)
        );
        str_append_char(&this_operation_rendered, ' ');
        StringBuffer* right_ref = already_rendered_chunks[operation.right];
        if (right_ref)
            str_concat(&this_operation_rendered, right_ref);
        else
            render_operand(&this_operation_rendered, expr.operands[operation.right]);
        str_append_char(&this_operation_rendered, ')');

        string_mem[i] = this_operation_rendered;
        if (left_ref)
            *left_ref = string_mem[i];
        else
            already_rendered_chunks[operation.left] = string_mem + i;
        if (right_ref) {
            *right_ref = string_mem[i];
        }
        else
            already_rendered_chunks[operation.right] = string_mem + i;
    }

    return string_mem[expr.n_operations - 1];
}

void
print_expression(Expression expr)
{
    StringBuffer buf = render_expr(expr);
    printf("%s", buf.data);
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
        case INST_EXPR:
            printf("EXPR: ");
            print_expression(inst.expr);
            break;
        case INST_FOR_LOOP:
            printf("FOR_LOOP: it=%s, iterable=", inst.for_loop.it.buffer);
            print_expression(inst.for_loop.iterable_expr);
            break;
    }
    printf("\n");
}

void
print_token(Token tok)
{
    printf("%s:%u:%u ", tok.loc.filename, tok.loc.line, tok.loc.col);
    print_token_type(tok.type);
    if (tok.type == TOK_OPERATOR) {
        printf(": ");
        print_operator_enum(tok.operator);
    }
    else if (tok.type == TOK_KEYWORD) {
        printf(": ");
        print_keyword(tok.keyword);
    }
    else if (tok.type == TOK_IDENTIFIER || tok.type == TOK_NUMBER || tok.type == TOK_STRING) {
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
        case TOK_IDENTIFIER:
            printf("IDENTIFIER");
            break;
        case TOK_DOT:
            printf("DOT");
            break;
    }
}

static const char*
operator_enum_to_cstr(Operator op)
{
    switch (op) {
        case OPERATOR_PLUS:
            return "PLUS";
        case OPERATOR_MINUS:
            return "MINUS";
        case OPERATOR_MULT:
            return "MULT";
        case OPERATOR_DIV:
            return "DIV";
        case OPERATOR_MOD:
            return "MOD";
        case OPERATOR_POW:
            return "POW";
        case OPERATOR_FLOORDIV:
            return "FLOORDIV";
        case OPERATOR_ASSIGNMENT:
            return "ASSIGNMENT";
        case OPERATOR_PLUS_ASSIGNMENT:
            return "PLUS_ASSIGNMENT";
        case OPERATOR_MINUS_ASSIGNMENT:
            return "MINUS_ASSIGNMENT";
        case OPERATOR_MULT_ASSIGNMENT:
            return "MULT_ASSIGNMENT";
        case OPERATOR_DIV_ASSIGNMENT:
            return "DIV_ASSIGNMENT";
        case OPERATOR_MOD_ASSIGNMENT:
            return "MOD_ASSIGNMENT";
        case OPERATOR_FLOORDIV_ASSIGNMENT:
            return "FLOORDIV_ASSIGNMENT";
        case OPERATOR_POW_ASSIGNMENT:
            return "POW_ASSIGNMENT";
        case OPERATOR_AND_ASSIGNMENT:
            return "AND_ASSIGNMENT";
        case OPERATOR_OR_ASSIGNMENT:
            return "OR_ASSIGNMENT";
        case OPERATOR_XOR_ASSIGNMENT:
            return "XOR_ASSIGNMENT";
        case OPERATOR_RSHIFT_ASSIGNMENT:
            return "RSHIFT_ASSIGNMENT";
        case OPERATOR_LSHIFT_ASSIGNMENT:
            return "LSHIFT_ASSIGNMENT";
        case OPERATOR_EQUAL:
            return "EQUAL";
        case OPERATOR_NOT_EQUAL:
            return "NOT_EQUAL";
        case OPERATOR_GREATER:
            return "GREATER";
        case OPERATOR_LESS:
            return "LESS";
        case OPERATOR_GREATER_EQUAL:
            return "GREATER_EQUAL";
        case OPERATOR_LESS_EQUAL:
            return "LESS_EQUAL";
        case OPERATOR_BITWISE_AND:
            return "BITWISE_AND";
        case OPERATOR_BITWISE_OR:
            return "BITWISE_OR";
        case OPERATOR_BITWISE_XOR:
            return "BITWISE_XOR";
        case OPERATOR_BITWISE_NOT:
            return "BITWISE_NOT";
        case OPERATOR_LSHIFT:
            return "LSHIFT";
        case OPERATOR_RSHIFT:
            return "RSHIFT";
    }
    return "UNKNOWN";
}

void
print_operator_enum(Operator op)
{
    printf("%s", operator_enum_to_cstr(op));
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
