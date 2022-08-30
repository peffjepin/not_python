#include "debug_common.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../src/lexer_helpers.h"

#define STRING_BUFFER_CAPACITY 2048

typedef struct {
    char data[STRING_BUFFER_CAPACITY];
    size_t length;
} StringBuffer;

static StringBuffer render_expr(Expression* expr);
StringBuffer render_it_group(ItGroup* it);

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
            str_concat_cstr(str, op.token.value);
            break;
        case OPERAND_EXPRESSION: {
            StringBuffer rendered_expr = render_expr(op.expr);
            str_concat(str, &rendered_expr);
            break;
        }
        case OPERAND_ENCLOSURE_LITERAL: {
            Enclosure* enclosure = op.enclosure;
            bool is_map = (enclosure->type == ENCLOSURE_DICT);
            render_enclosure_opening(str, enclosure->type);
            Expression** expressions = enclosure->expressions;
            for (size_t i = 0; i < enclosure->expressions_count; i++) {
                if (is_map) {
                    if (i % 2 == 1)
                        str_concat_cstr(str, ": ");
                    else if (i > 0)
                        str_concat_cstr(str, ", ");
                }
                else if (i > 0)
                    str_concat_cstr(str, ", ");
                StringBuffer rendered_element = render_expr(expressions[i]);
                str_concat(str, &rendered_element);
            }
            render_enclosure_closing(str, enclosure->type);
            break;
        }
        case OPERAND_COMPREHENSION: {
            Comprehension* comp = op.comp;
            bool is_map = (comp->type == ENCLOSURE_DICT);
            str_append_char(str, '\n');
            render_enclosure_opening(str, comp->type);
            // initial (key: value) expression
            if (is_map) {
                expr = render_expr(comp->mapped.key_expr);
                str_concat(str, &expr);
                str_concat_cstr(str, ": ");
                expr = render_expr(comp->mapped.val_expr);
                str_concat(str, &expr);
            }
            // initial expression
            else {
                expr = render_expr(comp->sequence.expr);
                str_concat(str, &expr);
            }
            // for loop bodies
            for (size_t i = 0; i < comp->body.loop_count; i++) {
                str_concat_cstr(str, "\n for ");
                StringBuffer its = render_it_group(comp->body.its[i]);
                str_concat(str, &its);
                str_concat_cstr(str, " in ");
                expr = render_expr(comp->body.iterables[i]);
                str_concat(str, &expr);
            }
            if (comp->body.if_expr) {
                str_concat_cstr(str, "\n if ");
                expr = render_expr(comp->body.if_expr);
                str_concat(str, &expr);
            }
            render_enclosure_closing(str, comp->type);
            break;
        }
        case OPERAND_ARGUMENTS: {
            str_append_char(str, '(');
            Arguments* args = op.args;
            size_t positional = 0;
            size_t kwds = 0;
            for (size_t i = 0; i < args->values_count; i++) {
                if (i > 0) str_concat_cstr(str, ", ");
                if (positional++ < args->n_positional) {
                    expr = render_expr(args->values[i]);
                    str_concat(str, &expr);
                }
                else {
                    str_concat_cstr(str, args->kwds[kwds++]);
                    str_append_char(str, '=');
                    expr = render_expr(args->values[i]);
                    str_concat(str, &expr);
                }
            }
            str_append_char(str, ')');
            break;
        }
        case OPERAND_SLICE: {
            str_concat_cstr(str, "slice(");
            Slice* slice = op.slice;
            bool need_comma = false;
            if (slice->start_expr) {
                str_concat_cstr(str, "start=");
                expr = render_expr(slice->start_expr);
                str_concat(str, &expr);
                need_comma = true;
            }
            if (slice->stop_expr) {
                if (need_comma) {
                    str_concat_cstr(str, ", ");
                    need_comma = false;
                }
                str_concat_cstr(str, "stop=");
                expr = render_expr(slice->stop_expr);
                str_concat(str, &expr);
                need_comma = true;
            }
            if (slice->step_expr) {
                if (need_comma) {
                    str_concat_cstr(str, ", ");
                }
                str_concat_cstr(str, "step=");
                expr = render_expr(slice->step_expr);
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
    StringBuffer string_mem[expr->operations_count];
    StringBuffer* already_rendered_chunks[expr->operands_count];
    memset(already_rendered_chunks, 0, sizeof(StringBuffer*) * expr->operands_count);
    memset(string_mem, 0, sizeof(StringBuffer) * expr->operations_count);

    Operand* operands = expr->operands;
    Operation* operations = expr->operations;

    if (expr->operations_count == 0) {
        StringBuffer buf = {0};
        render_operand(&buf, operands[0]);
        return buf;
    }

    for (size_t i = 0; i < expr->operations_count; i++) {
        Operation operation = operations[i];
        bool is_unary =
            (operation.op_type == OPERATOR_LOGICAL_NOT ||
             operation.op_type == OPERATOR_NEGATIVE ||
             operation.op_type == OPERATOR_POSITIVE ||
             operation.op_type == OPERATOR_BITWISE_NOT);

        StringBuffer this_operation_rendered = {0};

        str_append_char(&this_operation_rendered, '(');

        StringBuffer* left_ref = NULL;
        if (!is_unary) {
            left_ref = already_rendered_chunks[operation.left];
            if (left_ref)
                str_concat(&this_operation_rendered, left_ref);
            else
                render_operand(&this_operation_rendered, operands[operation.left]);
            str_append_char(&this_operation_rendered, ' ');
        }

        str_concat_cstr(&this_operation_rendered, (char*)op_to_cstr(operation.op_type));
        str_append_char(&this_operation_rendered, ' ');

        StringBuffer* right_ref = already_rendered_chunks[operation.right];
        if (right_ref)
            str_concat(&this_operation_rendered, right_ref);
        else
            render_operand(&this_operation_rendered, operands[operation.right]);

        str_append_char(&this_operation_rendered, ')');

        string_mem[i] = this_operation_rendered;
        if (!is_unary) {
            if (left_ref)
                *left_ref = string_mem[i];
            else
                already_rendered_chunks[operation.left] = string_mem + i;
        }
        if (right_ref) {
            *right_ref = string_mem[i];
        }
        else
            already_rendered_chunks[operation.right] = string_mem + i;
    }

    return string_mem[expr->operations_count - 1];
}

StringBuffer
render_it_group(ItGroup* it)
{
    StringBuffer buf = {0};
    for (size_t i = 0; i < it->identifiers_count; i++) {
        if (i > 0) str_concat_cstr(&buf, ", ");
        ItIdentifier id = it->identifiers[i];
        if (id.kind == IT_ID) {
            str_concat_cstr(&buf, id.name);
        }
        else {
            StringBuffer group_render = render_it_group(id.group);
            str_append_char(&buf, '(');
            str_concat(&buf, &group_render);
            str_append_char(&buf, ')');
        }
    }
    return buf;
}

void
print_block(Block block, int indent)
{
    for (size_t i = 0; i < block.stmts_count; i++) {
        print_statement(block.stmts + i, indent);
    }
}

// expects `indent` token to be in scope
#define indent_printf(fmtstr, ...) printf("%*s" fmtstr, indent, "", __VA_ARGS__)
#define indent_print(fmtstr) printf("%*s" fmtstr, indent, "")

void
print_import_path(ImportPath path)
{
    for (size_t i = 0; i < path.path_count; i++) {
        if (i > 0) printf(".");
        printf("%s", path.dotted_path[i]);
    }
}

void
print_statement(Statement* stmt, int indent)
{
    switch (stmt->kind) {
        case NULL_STMT: {
            printf("NULL STMT\n");
            break;
        }
        case STMT_EOF: {
            printf("EOF\n");
            break;
        }
        case STMT_EXPR: {
            indent_printf("%s\n", render_expr(stmt->expr).data);
            break;
        }
        case STMT_FOR_LOOP: {
            indent_printf(
                "for %s in %s:\n",
                render_it_group(stmt->for_loop->it).data,
                render_expr(stmt->for_loop->iterable).data
            );
            print_block(stmt->for_loop->body, indent + 4);
            break;
        }
        case STMT_IMPORT: {
            if (stmt->import_stmt->what) {
                printf("from ");
                print_import_path(stmt->import_stmt->from);
                printf(" import ");
                for (size_t i = 0; i < stmt->import_stmt->what_count; i++) {
                    if (i > 0) printf(", ");
                    printf("%s", stmt->import_stmt->what[i]);
                    char* as = stmt->import_stmt->as[i];
                    if (as) printf(" as %s", as);
                }
                printf("\n");
            }
            else {
                printf("import ");
                print_import_path(stmt->import_stmt->from);
                if (stmt->import_stmt->as)
                    printf(" as %s\n", stmt->import_stmt->as[0]);
                else
                    printf("\n");
            }
            break;
        }
        case STMT_WHILE: {
            indent_printf("while %s:\n", render_expr(stmt->while_stmt->condition).data);
            print_block(stmt->while_stmt->body, indent + 4);
            break;
        }
        case STMT_IF: {
            assert(0 && "STMT_IF printing is unimplemented");
            break;
        }
        case STMT_TRY: {
            assert(0 && "STMT_TRY printing is unimplemented");
            break;
        }
        case STMT_WITH: {
            assert(0 && "STMT_WITH printing is unimplemented");
            break;
        }
        case STMT_CLASS: {
            assert(0 && "STMT_CLASS printing is unimplemented");
            break;
        }
        case STMT_FUNCTION: {
            assert(0 && "STMT_FUNCTION printing is unimplemented");
            break;
        }
        case STMT_ASSIGNMENT: {
            assert(0 && "STMT_ASSIGNMENT printing is unimplemented");
            break;
        }
        case STMT_NO_OP: {
            indent_print("NO_OP\n");
            break;
        }
        default:
            assert(0 && "unimplemented");
    }
}

void
print_token(Token tok)
{
    printf(
        "%s:%u:%u %s",
        tok.loc.filename,
        tok.loc.line,
        tok.loc.col,
        token_type_to_cstr(tok.type)
    );
    if (tok.type == TOK_OPERATOR) {
        printf(": %s", op_to_cstr(tok.op));
    }
    else if (tok.type == TOK_KEYWORD) {
        printf(": %s", kw_to_cstr(tok.kw));
    }
    else if (tok.type == TOK_IDENTIFIER || tok.type == TOK_NUMBER || tok.type == TOK_STRING) {
        printf(": ");
        printf("%s", tok.value);
    }
    printf("\n");
}
