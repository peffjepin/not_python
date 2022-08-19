#include "debug_common.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define STRING_BUFFER_CAPACITY 1024

Arena* arena;

void
debugger_use_arena(Arena* a)
{
    arena = a;
}

typedef struct {
    char data[STRING_BUFFER_CAPACITY];
    size_t length;
} StringBuffer;

static StringBuffer render_expr(Expression expr);

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
            str_concat_cstr(str, op.constant.value);
            break;
        case OPERAND_STORAGE:
            str_concat_cstr(str, op.storage.identifier);
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
        str_concat_cstr(&this_operation_rendered, (char*)op_to_cstr(operation.op_type));
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
            printf("FOR_LOOP: it=%s, iterable=", inst.for_loop.it);
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
        printf(": %s", op_to_cstr(tok.value_ref));
    }
    else if (tok.type == TOK_KEYWORD) {
        printf(": %s", kw_to_cstr(tok.value_ref));
    }
    else if (tok.type == TOK_IDENTIFIER || tok.type == TOK_NUMBER || tok.type == TOK_STRING) {
        printf(": ");
        printf("%s", arena_get_string(arena, tok.value_ref));
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
