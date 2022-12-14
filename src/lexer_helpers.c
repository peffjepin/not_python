#include "lexer_helpers.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "diagnostics.h"

/*
 * Notes on vector definitions
 *
 * Vectors defined with these macros are allocated in dynamic arena memory
 * and are meant to be finalized (allocated into static arena memory) once
 * their size has been determined.
 *
 * Vectors defined with PTR_VECTOR_DEFINITION take their elements by pointers
 * and when finalized return type**;
 *
 * Vectors defined with VALUE_VECTOR_DEFINITION take their elements by value
 * and when finalized return type*
 */

#define PTR_VECTOR_DEFINITION(type, prefix)                                              \
    type##Vector prefix##_vector_init(Arena* arena)                                      \
    {                                                                                    \
        type##Vector vec = {                                                             \
            .arena = arena,                                                              \
            .capacity = 8,                                                               \
            .data = arena_dynamic_alloc(arena, sizeof(type*) * 8),                       \
        };                                                                               \
        return vec;                                                                      \
    }                                                                                    \
    void prefix##_vector_append(type##Vector* vec, type* ptr)                            \
    {                                                                                    \
        if (vec->count == vec->capacity) {                                               \
            vec->capacity *= 2;                                                          \
            vec->data = arena_dynamic_realloc(                                           \
                vec->arena, vec->data, (sizeof(type*) * vec->capacity)                   \
            );                                                                           \
        }                                                                                \
        vec->data[vec->count++] = ptr;                                                   \
    }                                                                                    \
    type** prefix##_vector_finalize(type##Vector* vec)                                   \
    {                                                                                    \
        return arena_dynamic_finalize(vec->arena, vec->data);                            \
    }

#define VALUE_VECTOR_DEFINITION(type, prefix)                                            \
    type##Vector prefix##_vector_init(Arena* arena)                                      \
    {                                                                                    \
        type##Vector vec = {                                                             \
            .arena = arena,                                                              \
            .capacity = 8,                                                               \
            .data = arena_dynamic_alloc(arena, sizeof(type) * 8),                        \
        };                                                                               \
        return vec;                                                                      \
    }                                                                                    \
    void prefix##_vector_append(type##Vector* vec, type value)                           \
    {                                                                                    \
        if (vec->count == vec->capacity) {                                               \
            vec->capacity *= 2;                                                          \
            vec->data = arena_dynamic_realloc(                                           \
                vec->arena, vec->data, sizeof(type) * vec->capacity                      \
            );                                                                           \
        }                                                                                \
        vec->data[vec->count++] = value;                                                 \
    }                                                                                    \
    type* prefix##_vector_finalize(type##Vector* vec)                                    \
    {                                                                                    \
        return arena_dynamic_finalize(vec->arena, vec->data);                            \
    }

PTR_VECTOR_DEFINITION(Expression, expr)
PTR_VECTOR_DEFINITION(ItGroup, itgroup)
PTR_VECTOR_DEFINITION(Statement, stmt)
VALUE_VECTOR_DEFINITION(SourceString, str)
VALUE_VECTOR_DEFINITION(ItIdentifier, itid)
VALUE_VECTOR_DEFINITION(ElifStatement, elif)
VALUE_VECTOR_DEFINITION(ExceptStatement, except)
VALUE_VECTOR_DEFINITION(TypeInfo, typing)

void
out_of_memory(void)
{
    fprintf(stderr, "ERROR: out of memory");
    exit(1);
}

const char*
file_namespace(Arena* arena, const char* filepath)
{
    // TODO: portability / make more robust
    size_t length = strlen(filepath);
    size_t last_dot = length;
    size_t last_slash = 0;
    for (size_t i = 0; i < length; i++) {
        if (filepath[i] == '.') last_dot = i;
        if (filepath[i] == '/') last_slash = i;
    }
    char* rtval = arena_alloc(arena, 1 + last_dot - last_slash);
    size_t start = (last_slash) ? last_slash + 1 : 0;
    memcpy(rtval, filepath + start, last_dot - start);
    rtval[last_dot - start] = '_';
    return rtval;
}

#define TOKEN_MAX 20

static_assert(TOKEN_MAX == 20, "new token needs to be added to TOKEN_TYPE_TO_CSTR_TABLE");

static const char* TOKEN_TYPE_TO_CSTR_TABLE[TOKEN_MAX] = {
    [TOK_KEYWORD] = "KEYWORD",
    [TOK_COMMA] = ",",
    [TOK_COLON] = ":",
    [TOK_STRING] = "STRING",
    [TOK_NUMBER] = "NUMBER",
    [TOK_OPERATOR] = "OPERATOR",
    [TOK_NEWLINE] = "NEWLINE",
    [TOK_OPEN_PARENS] = "(",
    [TOK_CLOSE_PARENS] = ")",
    [TOK_OPEN_SQUARE] = "[",
    [TOK_CLOSE_SQUARE] = "]",
    [TOK_OPEN_CURLY] = "{",
    [TOK_CLOSE_CURLY] = "}",
    [TOK_IDENTIFIER] = "IDENTIFIER",
    [TOK_DOT] = ".",
    [TOK_ARROW] = "->",
    [TOK_EOF] = "EOF",
};

const char*
token_type_to_cstr(TokenType type)
{
    assert(type < TOKEN_MAX && "token type not in table");
    return TOKEN_TYPE_TO_CSTR_TABLE[type];
}

Token
tq_peek(TokenQueue* tq, size_t offset)
{
    assert(tq->length && "peeking empty queue");
    assert(tq->length > offset && "peeking past the end of queue");
    return tq->tokens[(tq->head + offset) % TOKEN_QUEUE_CAPACITY];
}

Token
tq_consume(TokenQueue* tq)
{
    assert(tq->length && "consuming empty queue");
    tq->length -= 1;
    return tq->tokens[tq->head++ % TOKEN_QUEUE_CAPACITY];
}

void
tq_push(TokenQueue* tq, Token token)
{
    assert(tq->length < TOKEN_QUEUE_CAPACITY && "pushing to full queue");
    tq->tokens[(tq->head + tq->length++) % TOKEN_QUEUE_CAPACITY] = token;
}

void
tq_discard(TokenQueue* tq)
{
    tq->length--;
    tq->head++;
}

#define INDENT_TOP(stack) stack->values[stack->count - 1]
#define INDENT_PUT(stack, indent) stack->values[stack->count++] = indent

const char*
indent_check(IndentationStack* stack, Location loc, bool begin_block)
{
    assert(loc.col > 0 && "bad location");
    // first statement checked
    if (stack->count == 0) {
        if (loc.col != 1) {
            return "the top level of a file should not be indented";
        }
        else
            INDENT_PUT(stack, loc.col);
    }
    // indentation unchanged
    else if (INDENT_TOP(stack) == loc.col)
        return NULL;
    // indentation went down
    else if (INDENT_TOP(stack) > loc.col) {
        assert(stack->count > 1 && "indentation stack corrupted");
        for (int i = stack->count - 2; i >= 1; i--) {
            if (stack->values[i] == loc.col) {
                stack->count = i + 1;
                return NULL;
            }
            if (stack->values[i] < loc.col) return "inconsistent indentation";
        }
    }
    // indentation went up
    else {
        if (stack->count == INDENTATION_MAX)
            return "max indenation level (" INDENTATION_MAX_STR ") exceeded";
        if (!begin_block) return "unexpected indentation";
        INDENT_PUT(stack, loc.col);
    }

    return NULL;
}

LexicalScope*
scope_init(Arena* arena)
{
    LexicalScope* scope = arena_alloc(arena, sizeof(LexicalScope));
    scope->hm = symbol_hm_init(arena);
    return scope;
}

LexicalScope*
scope_stack_peek(LexicalScopeStack* stack)
{
    assert(stack->count && "peeking empty stack");
    return stack->scopes[stack->count - 1];
}

void
scope_stack_push(LexicalScopeStack* stack, LexicalScope* scope)
{
    assert(stack->count < SCOPE_STACK_MAX && "pushing to full stack");
    stack->scopes[stack->count++] = scope;
}

LexicalScope*
scope_stack_pop(LexicalScopeStack* stack)
{
    assert(stack->count && "popping empty stack");
    LexicalScope* scope = stack->scopes[--stack->count];
    symbol_hm_finalize(&scope->hm);
    return scope;
}

Symbol*
get_symbol_from_scopes(LexicalScopeStack stack, SourceString identifier)
{
    bool closure = false;
    for (size_t i = stack.count - 1; i > 0; i--) {
        LexicalScope* locals = stack.scopes[i];
        Symbol* sym = symbol_hm_get(&locals->hm, identifier);
        if (sym) {
            if (closure && sym->kind == SYM_VARIABLE) {
                if (sym->variable->kind == VAR_SELF ||
                    sym->variable->kind == VAR_CLOSURE_ARGUMENT ||
                    sym->variable->kind == VAR_CLOSURE)
                    return sym;
                if (sym->variable->kind == VAR_ARGUMENT)
                    sym->variable->kind = VAR_CLOSURE_ARGUMENT;
                else
                    sym->variable->kind = VAR_CLOSURE;
            }
            return sym;
        }
        else if (locals->kind != SCOPE_CLOSURE_CHILD)
            break;
        closure = true;
    }

    return symbol_hm_get(&stack.scopes[0]->hm, identifier);
}

void
operation_vector_push(OperationVector* vec, Operation operation)
{
    if (vec->capacity == vec->count) {
        vec->capacity += 16;
        vec->operations = realloc(vec->operations, sizeof(Operation) * vec->capacity);
        if (!vec->operations) out_of_memory();
    }
    vec->operations[vec->count++] = operation;
}

ExpressionTable
et_init(Arena* arena)
{
    ExpressionTable et = {
        .arena = arena,
        .operands_capacity = 8,
        .operands = arena_dynamic_alloc(arena, sizeof(Operand) * 8),
    };
    return et;
}

void
et_push_operand(ExpressionTable* et, Operand operand)
{
    if (et->operands_count == et->operands_capacity) {
        et->operands_capacity *= 2;
        et->operands = arena_dynamic_realloc(
            et->arena, et->operands, sizeof(Operand) * et->operands_capacity
        );
    }
    et->operands[et->operands_count++] = operand;
    et->previous = ET_OPERAND;
}

void
et_push_operation(ExpressionTable* et, Operation operation)
{
    unsigned int precedence = PRECEDENCE_TABLE[operation.op_type];
    OperationVector* vec = et->operation_vectors + precedence;
    operation_vector_push(vec, operation);
    et->operations_count += 1;
    et->previous = ET_OPERATION;
}

void
et_push_operation_type(ExpressionTable* et, Operator op_type, Location* loc)
{
    Operation operation = {
        .loc = loc,
        .op_type = op_type,
        .left = et->operands_count - 1,
        .right = et->operands_count};
    et_push_operation(et, operation);
}

Expression*
et_to_expr(ExpressionTable* et)
{
    assert(et->previous != ET_NONE && "got an empty expression");
    assert(et->previous == ET_OPERAND && "expression ended on an operator token");

    const unsigned int POW_PREC = PRECEDENCE_TABLE[OPERATOR_POW];

    Expression* expr = arena_alloc(et->arena, sizeof(Expression));
    expr->operations = arena_alloc(et->arena, sizeof(Operation) * et->operations_count);
    expr->operations_count = et->operations_count;
    expr->operands = arena_dynamic_finalize(et->arena, et->operands);
    expr->operands_count = et->operands_count;

    size_t sorted_count = 0;
    for (int prec = MAX_PRECEDENCE; prec >= 0; prec--) {
        if (sorted_count == et->operations_count) break;
        OperationVector vec = et->operation_vectors[prec];
        if (vec.count == 0) continue;

        // OPERATOR_POW proceeds right->left
        if ((unsigned)prec == POW_PREC) {
            for (size_t i = 0; i < vec.count; i++)
                expr->operations[sorted_count++] = vec.operations[vec.count - 1 - i];
        }
        // everything else proceeds left->right so we can just copy
        else {
            memcpy(
                expr->operations + sorted_count,
                vec.operations,
                sizeof(Operation) * vec.count
            );
            sorted_count += vec.count;
        }
        free(vec.operations);
    }
    return expr;
}

NpType
cstr_to_python_type(const char* cstr)
{
    switch (cstr[0]) {
        case 'F':
            if (strcmp(cstr, "Function") == 0) return NPTYPE_FUNCTION;
            return NPTYPE_OBJECT;
        case 'C':
            if (strcmp(cstr, "Callable") == 0) return NPTYPE_FUNCTION;
            return NPTYPE_OBJECT;
        case 'N':
            if (strcmp(cstr, "None") == 0) return NPTYPE_NONE;
            return NPTYPE_OBJECT;
        case 'L':
            if (strcmp(cstr, "List") == 0) return NPTYPE_LIST;
            return NPTYPE_OBJECT;
        case 'D':
            if (strcmp(cstr, "Dict") == 0) return NPTYPE_DICT;
            return NPTYPE_OBJECT;
        case 'T':
            if (strcmp(cstr, "Tuple") == 0) return NPTYPE_TUPLE;
            return NPTYPE_OBJECT;
        case 'i':
            if (strcmp(cstr, "int") == 0) return NPTYPE_INT;
            return NPTYPE_OBJECT;
        case 'f':
            if (strcmp(cstr, "float") == 0) return NPTYPE_FLOAT;
            return NPTYPE_OBJECT;
        case 's':
            if (strcmp(cstr, "str") == 0) return NPTYPE_STRING;
            return NPTYPE_OBJECT;
        case 'b':
            if (strcmp(cstr, "bool") == 0) return NPTYPE_BOOL;
            return NPTYPE_OBJECT;
        default:
            return NPTYPE_OBJECT;
    }
    UNREACHABLE();
}
