#include "debug_common.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "lexer_helpers.h"

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
            if (op.token.type == TOK_KEYWORD && op.token.kw == KW_FALSE) {
                str_concat_cstr(str, "False");
            }
            else if (op.token.type == TOK_KEYWORD && op.token.kw == KW_TRUE) {
                str_concat_cstr(str, "True");
            }
            else {
                str_concat_cstr(str, op.token.value.data);
            }
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
                    str_concat_cstr(str, args->kwds[kwds++].data);
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
            str_concat_cstr(&buf, id.name.data);
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
        print_statement(block.stmts[i], indent);
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
        printf("%s", path.dotted_path[i].data);
    }
}

static StringBuffer render_type_info(TypeInfo info);

static StringBuffer
render_inner_type_info(TypeInfoInner* inner)
{
    StringBuffer str = {0};
    str_append_char(&str, '[');
    for (size_t i = 0; i < inner->count; i++) {
        if (i > 0) str_concat_cstr(&str, ", ");
        StringBuffer this_info = render_type_info(inner->types[i]);
        str_concat(&str, &this_info);
    }
    str_append_char(&str, ']');
    return str;
}

static StringBuffer
render_type_info(TypeInfo info)
{
    StringBuffer str = {0};
    switch (info.type) {
        case NPTYPE_UNTYPED:
            str_concat_cstr(&str, "Untyped");
            break;
        case NPTYPE_NONE:
            str_concat_cstr(&str, "None");
            break;
        case NPTYPE_INT:
            str_concat_cstr(&str, "int");
            break;
        case NPTYPE_FLOAT:
            str_concat_cstr(&str, "float");
            break;
        case NPTYPE_STRING:
            str_concat_cstr(&str, "str");
            break;
        case NPTYPE_FUNCTION:
            str_concat_cstr(&str, "Function[[");
            for (size_t i = 0; i < info.sig->params_count; i++) {
                if (i > 0) str_concat_cstr(&str, ", ");
                str_concat_cstr(&str, render_type_info(info.sig->types[i]).data);
            }
            str_concat_cstr(&str, "], ");
            str_concat_cstr(&str, render_type_info(info.sig->return_type).data);
            str_concat_cstr(&str, "]");
            break;
        case NPTYPE_LIST:
            str_concat_cstr(&str, "List");
            str_concat_cstr(&str, render_inner_type_info(info.inner).data);
            break;
        case NPTYPE_DICT:
            str_concat_cstr(&str, "Dict");
            str_concat_cstr(&str, render_inner_type_info(info.inner).data);
            break;
        case NPTYPE_TUPLE:
            str_concat_cstr(&str, "Tuple");
            str_concat_cstr(&str, render_inner_type_info(info.inner).data);
            break;
        case NPTYPE_OBJECT:
            str_concat_cstr(&str, info.cls->name.data);
            break;
        default:
            str_concat_cstr(&str, "UnrecognizedTypeInfo");
            break;
    }
    return str;
}

void
print_class_definition(ClassStatement* cls, int indent)
{
    indent_printf("class %s", cls->name.data);
    if (cls->base.data) {
        printf("(%s)", cls->base.data);
    }
    printf(":\n");
}

void
print_function_definition(FunctionStatement* func, int indent)
{
    indent_printf("def %s(", func->name.data);
    if (func->self_param.data) {
        printf("%s: %s", func->self_param.data, render_type_info(func->self_type).data);
    }
    size_t positional_count = func->sig.params_count - func->sig.defaults_count;
    for (size_t i = 0; i < positional_count; i++) {
        if (i > 0 || func->self_param.data) printf(", ");
        printf(
            "%s: %s", func->sig.params[i].data, render_type_info(func->sig.types[i]).data
        );
    }
    for (size_t i = 0; i < func->sig.defaults_count; i++) {
        if (i > 0 || positional_count > 0) printf(", ");
        printf(
            "%s: %s = %s",
            func->sig.params[positional_count + i].data,
            render_type_info(func->sig.types[positional_count + i]).data,
            render_expr(func->sig.defaults[i]).data
        );
    }
    printf(") -> %s:\n", render_type_info(func->sig.return_type).data);
}

void
print_statement(Statement* stmt, int indent)
{
    switch (stmt->kind) {
        case NULL_STMT: {
            printf("NULL STMT\n");
            break;
        }
        case STMT_RETURN: {
            indent_printf("return %s\n", render_expr(stmt->return_expr).data);
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
            if (stmt->import->what) {
                printf("from ");
                print_import_path(stmt->import->from);
                printf(" import ");
                for (size_t i = 0; i < stmt->import->what_count; i++) {
                    if (i > 0) printf(", ");
                    printf("%s", stmt->import->what[i].data);
                    char* as = stmt->import->as[i].data;
                    if (as) printf(" as %s", as);
                }
                printf("\n");
            }
            else {
                printf("import ");
                print_import_path(stmt->import->from);
                if (stmt->import->as)
                    printf(" as %s\n", stmt->import->as[0].data);
                else
                    printf("\n");
            }
            break;
        }
        case STMT_WHILE: {
            indent_printf("while %s:\n", render_expr(stmt->while_loop->condition).data);
            print_block(stmt->while_loop->body, indent + 4);
            break;
        }
        case STMT_IF: {
            indent_printf("if %s:\n", render_expr(stmt->conditional->condition).data);
            print_block(stmt->conditional->body, indent + 4);
            for (size_t i = 0; i < stmt->conditional->elifs_count; i++) {
                indent_printf(
                    "elif %s:\n", render_expr(stmt->conditional->elifs[i].condition).data
                );
                print_block(stmt->conditional->elifs[i].body, indent + 4);
            }
            if (stmt->conditional->else_body.stmts_count > 0) {
                indent_print("else:\n");
                print_block(stmt->conditional->else_body, indent + 4);
            }
            break;
        }
        case STMT_TRY: {
            // try
            indent_print("try:\n");
            print_block(stmt->try_stmt->try_body, indent + 4);
            // excepts
            for (size_t i = 0; i < stmt->try_stmt->excepts_count; i++) {
                ExceptStatement except = stmt->try_stmt->excepts[i];
                indent_print("except ");
                if (except.exceptions_count > 1) {
                    printf("(");
                    for (size_t j = 0; j < except.exceptions_count; j++) {
                        if (j > 0) printf(", ");
                        printf("%s", except.exceptions[j].data);
                    }
                    printf(")");
                }
                else
                    printf("%s", except.exceptions[0].data);
                if (except.as.data != NULL) printf(" as %s", except.as.data);
                printf(":\n");
                print_block(except.body, indent + 4);
            }
            // else
            if (stmt->try_stmt->else_body.stmts_count > 0) {
                indent_print("else:\n");
                print_block(stmt->try_stmt->else_body, indent + 4);
            }
            // finally
            if (stmt->try_stmt->finally_body.stmts_count > 0) {
                indent_print("finally:\n");
                print_block(stmt->try_stmt->finally_body, indent + 4);
            }
            break;
        }
        case STMT_WITH: {
            indent_printf("with %s", render_expr(stmt->with->ctx_manager).data);
            if (stmt->with->as.data) printf(" as %s", stmt->with->as.data);
            printf(":\n");
            print_block(stmt->with->body, indent + 4);
            break;
        }
        case STMT_CLASS: {
            print_class_definition(stmt->cls, indent);
            print_block(stmt->cls->body, indent + 4);
            break;
        }
        case STMT_FUNCTION: {
            print_function_definition(stmt->func, indent);
            print_block(stmt->func->body, indent + 4);
            break;
        }
        case STMT_ASSIGNMENT: {
            printf(
                "%s %s %s\n",
                render_expr(stmt->assignment->storage).data,
                op_to_cstr(stmt->assignment->op_type),
                render_expr(stmt->assignment->value).data
            );
            break;
        }
        case STMT_NO_OP: {
            indent_print("NO_OP\n");
            break;
        }
        case STMT_ANNOTATION: {
            indent_printf(
                "%s: %s",
                stmt->annotation->identifier.data,
                render_type_info(stmt->annotation->type).data
            );
            if (stmt->annotation->initial)
                printf(" = %s", render_expr(stmt->annotation->initial).data);
            printf("\n");
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
        tok.loc->filepath,
        tok.loc->line,
        tok.loc->col,
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
        printf("%s", tok.value.data);
    }
    printf("\n");
}

void
print_symbol(Symbol sym, int indent)
{
    switch (sym.kind) {
        case SYM_VARIABLE:
            indent_printf("%s\n", sym.identifier.data);
            break;
        case SYM_FUNCTION:
            indent_printf("%s:\n", sym.identifier.data);
            for (size_t i = 0; i < sym.func->scope->hm.elements_count; i++) {
                Symbol inner = sym.func->scope->hm.elements[i];
                print_symbol(inner, indent + 4);
            }
            break;
        case SYM_CLASS:
            indent_printf("%s:\n", sym.identifier.data);
            for (size_t i = 0; i < sym.cls->scope->hm.elements_count; i++) {
                Symbol inner = sym.cls->scope->hm.elements[i];
                print_symbol(inner, indent + 4);
            }
            break;
        case SYM_MEMBER:
            indent_printf(
                "%s: %s\n", sym.identifier.data, render_type_info(*sym.member_type).data
            );
            break;
    }
}

void
print_scopes(Lexer* lexer)
{
    LexicalScope* top_level = lexer->top_level;
    for (size_t i = 0; i < top_level->hm.elements_count; i++) {
        Symbol sym = top_level->hm.elements[i];
        print_symbol(sym, 0);
    }
}
