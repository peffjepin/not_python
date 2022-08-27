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

static StringBuffer render_expr(Expression* expr);

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
render_enclosure_opening(StringBuffer* str, EnclosureType type)
{
    switch (type) {
        case ENCLOSURE_DICT:
            str_append_char(str, '{');
            break;
        case ENCLOSURE_LIST:
            str_append_char(str, '[');
            break;
        case ENCLOSURE_TUPLE:
            str_append_char(str, '(');
            break;
    }
}

static void
render_enclosure_closing(StringBuffer* str, EnclosureType type)
{
    switch (type) {
        case ENCLOSURE_DICT:
            str_append_char(str, '}');
            break;
        case ENCLOSURE_LIST:
            str_append_char(str, ']');
            break;
        case ENCLOSURE_TUPLE:
            str_append_char(str, ')');
            break;
    }
}

static void
render_operand(StringBuffer* str, Operand op)
{
    StringBuffer expr;

    switch (op.kind) {
        case OPERAND_TOKEN:
            str_concat_cstr(str, (char*)arena_get_memory(arena, op.token.ref));
            break;
        case OPERAND_EXPRESSION: {
            StringBuffer rendered_expr =
                render_expr((Expression*)arena_get_memory(arena, op.ref));
            str_concat(str, &rendered_expr);
            break;
        }
        case OPERAND_ENCLOSURE_LITERAL: {
            Enclosure* enclosure = (Enclosure*)arena_get_memory(arena, op.ref);
            bool is_map = (enclosure->type == ENCLOSURE_DICT);
            render_enclosure_opening(str, enclosure->type);
            Expression* expressions = arena_get_memory(arena, enclosure->expressions.ref);
            for (size_t i = 0; i < enclosure->expressions.length; i++) {
                if (is_map) {
                    if (i % 2 == 1)
                        str_concat_cstr(str, ": ");
                    else if (i > 0)
                        str_concat_cstr(str, ", ");
                }
                else if (i > 0)
                    str_concat_cstr(str, ", ");
                StringBuffer rendered_element = render_expr(expressions + i);
                str_concat(str, &rendered_element);
            }
            render_enclosure_closing(str, enclosure->type);
            break;
        }
        case OPERAND_COMPREHENSION: {
            Comprehension* comp = arena_get_memory(arena, op.ref);
            bool is_map = (comp->type == ENCLOSURE_DICT);
            str_append_char(str, '\n');
            render_enclosure_opening(str, comp->type);
            // initial (key: value) expression
            if (is_map) {
                expr = render_expr(
                    (Expression*)arena_get_memory(arena, comp->mapped.key_expr)
                );
                str_concat(str, &expr);
                str_concat_cstr(str, ": ");
                expr = render_expr(
                    (Expression*)arena_get_memory(arena, comp->mapped.val_expr)
                );
                str_concat(str, &expr);
            }
            // initial expression
            else {
                expr =
                    render_expr((Expression*)arena_get_memory(arena, comp->sequence.expr)
                    );
                str_concat(str, &expr);
            }
            // for loop bodies
            for (size_t i = 0; i < comp->body.nesting; i++) {
                str_concat_cstr(str, "\n for ");
                expr =
                    render_expr((Expression*)arena_get_memory(arena, comp->body.its[i]));
                str_concat(str, &expr);
                str_concat_cstr(str, " in ");
                expr = render_expr(
                    (Expression*)arena_get_memory(arena, comp->body.iterables[i])
                );
                str_concat(str, &expr);
            }
            if (comp->body.has_if) {
                str_concat_cstr(str, "\n if ");
                expr =
                    render_expr((Expression*)arena_get_memory(arena, comp->body.if_expr));
                str_concat(str, &expr);
            }
            render_enclosure_closing(str, comp->type);
            break;
        }
        case OPERAND_ARGUMENTS: {
            str_append_char(str, '(');
            Arguments* args = (Arguments*)arena_get_memory(arena, op.ref);
            size_t positional = 0;
            size_t kwds = 0;
            for (size_t i = 0; i < args->length; i++) {
                if (i > 0) str_concat_cstr(str, ", ");
                if (positional++ < args->n_positional) {
                    expr = render_expr(
                        (Expression*)arena_get_memory(arena, args->value_refs[i])
                    );
                    str_concat(str, &expr);
                }
                else {
                    str_concat_cstr(
                        str, (char*)arena_get_memory(arena, args->kwds[kwds++].ref)
                    );
                    str_append_char(str, '=');
                    expr = render_expr(
                        (Expression*)arena_get_memory(arena, args->value_refs[i])
                    );
                    str_concat(str, &expr);
                }
            }
            str_append_char(str, ')');
            break;
        }
        case OPERAND_SLICE: {
            str_concat_cstr(str, "slice(");
            Slice* slice = (Slice*)arena_get_memory(arena, op.ref);
            bool need_comma = false;
            if (!slice->use_default_start) {
                str_concat_cstr(str, "start=");
                expr = render_expr(
                    (Expression*)arena_get_memory(arena, slice->start_expr_ref)
                );
                str_concat(str, &expr);
                need_comma = true;
            }
            if (!slice->use_default_stop) {
                if (need_comma) {
                    str_concat_cstr(str, ", ");
                    need_comma = false;
                }
                str_concat_cstr(str, "stop=");
                expr =
                    render_expr((Expression*)arena_get_memory(arena, slice->stop_expr_ref)
                    );
                str_concat(str, &expr);
                need_comma = true;
            }
            if (!slice->use_default_step) {
                if (need_comma) {
                    str_concat_cstr(str, ", ");
                }
                str_concat_cstr(str, "step=");
                expr =
                    render_expr((Expression*)arena_get_memory(arena, slice->step_expr_ref)
                    );
                str_concat(str, &expr);
                need_comma = true;
            }
            str_append_char(str, ')');
            break;
        }
        default:
            assert(0 && "not implemented");
    }
}

static StringBuffer
render_expr(Expression* expr)
{
    StringBuffer string_mem[expr->operations.length];
    StringBuffer* already_rendered_chunks[expr->operands.length];
    memset(already_rendered_chunks, 0, sizeof(StringBuffer*) * expr->operands.length);
    memset(string_mem, 0, sizeof(StringBuffer) * expr->operations.length);

    Operand* operands = arena_get_memory(arena, expr->operands.ref);
    Operation* operations = arena_get_memory(arena, expr->operations.ref);

    if (expr->operations.length == 0) {
        StringBuffer buf = {0};
        render_operand(&buf, operands[0]);
        return buf;
    }

    for (size_t i = 0; i < expr->operations.length; i++) {
        Operation operation = operations[i];
        StringBuffer this_operation_rendered = {0};

        str_append_char(&this_operation_rendered, '(');
        StringBuffer* left_ref = already_rendered_chunks[operation.left];
        if (left_ref)
            str_concat(&this_operation_rendered, left_ref);
        else
            render_operand(&this_operation_rendered, operands[operation.left]);
        str_append_char(&this_operation_rendered, ' ');
        str_concat_cstr(&this_operation_rendered, (char*)op_to_cstr(operation.op_type));
        str_append_char(&this_operation_rendered, ' ');
        StringBuffer* right_ref = already_rendered_chunks[operation.right];
        if (right_ref)
            str_concat(&this_operation_rendered, right_ref);
        else
            render_operand(&this_operation_rendered, operands[operation.right]);
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

    return string_mem[expr->operations.length - 1];
}

void
print_expression(Expression* expr)
{
    StringBuffer buf = render_expr(expr);
    printf("%s", buf.data);
}

void
print_statement(Statement* stmt)
{
    switch (stmt->kind) {
        case NULL_STMT:
            printf("NULL STMT");
            break;
        case STMT_EOF:
            printf("EOF");
            break;
        case STMT_EXPR:
            printf("EXPR: ");
            print_expression((Expression*)arena_get_memory(arena, stmt->expr_ref));
            break;
        default:
            assert(0 && "unimplemented");
    }
    printf("\n");
}

void
print_token(Token tok)
{
    printf("%s:%u:%u ", tok.loc.filename, tok.loc.line, tok.loc.col);
    print_token_type(tok.type);
    if (tok.type == TOK_OPERATOR) {
        printf(": %s", op_to_cstr(tok.value));
    }
    else if (tok.type == TOK_KEYWORD) {
        printf(": %s", kw_to_cstr(tok.value));
    }
    else if (tok.type == TOK_IDENTIFIER || tok.type == TOK_NUMBER || tok.type == TOK_STRING) {
        printf(": ");
        printf("%s", (char*)arena_get_memory(arena, tok.ref));
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
