#include "lexer_helpers.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODO: vector types allocated in dynamic arena memory could probably be
// implemented very easily with a macro

void
out_of_memory(void)
{
    fprintf(stderr, "ERROR: out of memory");
    exit(1);
}

static const char* TOKEN_TYPE_TO_CSTR_TABLE[19] = {
    [TOK_KEYWORD] = "keyword",
    [TOK_COMMA] = ",",
    [TOK_COLON] = ":",
    [TOK_STRING] = "string",
    [TOK_NUMBER] = "number",
    [TOK_OPERATOR] = "operator",
    [TOK_NEWLINE] = "\n",
    [TOK_OPEN_PARENS] = "(",
    [TOK_CLOSE_PARENS] = ")",
    [TOK_OPEN_SQUARE] = "[",
    [TOK_CLOSE_SQUARE] = "]",
    [TOK_OPEN_CURLY] = "{",
    [TOK_CLOSE_CURLY] = "}",
    [TOK_IDENTIFIER] = "identifier",
    [TOK_DOT] = ".",
    [TOK_EOF] = "EOF",
};

const char*
token_type_to_cstr(TokenType type)
{
    assert(type < 19 && "token type not in table");
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

ExpressionVector
expr_vector_init(Arena* arena)
{
    ExpressionVector vec = {.arena = arena};
    vec.data = arena_dynamic_alloc(arena, &vec.bytes);
    vec.capacity = vec.bytes / sizeof(Expression*);
    return vec;
}

void
expr_vector_append(ExpressionVector* vec, Expression* expr)
{
    if (vec->count == vec->capacity) {
        vec->data = arena_dynamic_grow(vec->arena, vec->data, &vec->bytes);
        vec->capacity = vec->bytes / sizeof(Expression*);
    }
    vec->data[vec->count++] = expr;
}

Expression**
expr_vector_finalize(ExpressionVector* vec)
{
    return arena_dynamic_finalize(
        vec->arena, vec->data, sizeof(Expression*) * vec->count
    );
}

StringVector
str_vector_init(Arena* arena)
{
    StringVector vec = {.arena = arena};
    vec.data = arena_dynamic_alloc(arena, &vec.bytes);
    vec.capacity = vec.bytes / sizeof(char*);
    return vec;
}

void
str_vector_append(StringVector* vec, char* str)
{
    if (vec->count == vec->capacity) {
        vec->data = arena_dynamic_grow(vec->arena, vec->data, &vec->bytes);
        vec->capacity = vec->bytes / sizeof(char*);
    }
    vec->data[vec->count++] = str;
}

char**
str_vector_finalize(StringVector* vec)
{
    return arena_dynamic_finalize(vec->arena, vec->data, sizeof(char*) * vec->count);
}

ItIdentifierVector
itid_vector_init(Arena* arena)
{
    ItIdentifierVector vec = {.arena = arena};
    vec.data = arena_dynamic_alloc(arena, &vec.bytes);
    vec.capacity = vec.bytes / sizeof(ItIdentifier);
    return vec;
}

void
itid_vector_append(ItIdentifierVector* vec, ItIdentifier itid)
{
    if (vec->count == vec->capacity) {
        vec->data = arena_dynamic_grow(vec->arena, vec->data, &vec->bytes);
        vec->capacity = vec->bytes / sizeof(ItIdentifier);
    }
    vec->data[vec->count++] = itid;
}

ItIdentifier*
itid_vector_finalize(ItIdentifierVector* vec)
{
    return arena_dynamic_finalize(
        vec->arena, vec->data, sizeof(ItIdentifier) * vec->count
    );
}

ItGroupVector
itgroup_vector_init(Arena* arena)
{
    ItGroupVector vec = {.arena = arena};
    vec.data = arena_dynamic_alloc(arena, &vec.bytes);
    vec.capacity = vec.bytes / sizeof(ItGroup*);
    return vec;
}

void
itgroup_vector_append(ItGroupVector* vec, ItGroup* group)
{
    if (vec->count == vec->capacity) {
        vec->data = arena_dynamic_grow(vec->arena, vec->data, &vec->bytes);
        vec->capacity = vec->bytes / sizeof(ItGroup*);
    }
    vec->data[vec->count++] = group;
}

ItGroup**
itgroup_vector_finalize(ItGroupVector* vec)
{
    return arena_dynamic_finalize(vec->arena, vec->data, sizeof(ItGroup*) * vec->count);
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
    ExpressionTable et = {.arena = arena};
    et.operands = arena_dynamic_alloc(arena, &et.operands_nbytes);
    et.operands_capacity = et.operands_nbytes / sizeof(Operand);
    return et;
}

void
et_push_operand(ExpressionTable* et, Operand operand)
{
    if (et->operands_count == et->operands_capacity) {
        et->operands = arena_dynamic_grow(et->arena, et->operands, &et->operands_nbytes);
        et->operands_capacity = et->operands_nbytes / sizeof(Operand);
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
et_push_operation_type(ExpressionTable* et, Operator op_type)
{
    Operation operation = {
        .op_type = op_type, .left = et->operands_count - 1, .right = et->operands_count};
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
    expr->operands = arena_dynamic_finalize(
        et->arena, et->operands, sizeof(Operand) * et->operands_count
    );
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

// TODO: portability
size_t
filename_offset(const char* filepath)
{
    size_t length = strlen(filepath);
    for (size_t i = 0; i < length; i++) {
        size_t index = length - 1 - i;
        if (filepath[index] == '/') return index + 1;
    }
    return 0;
}
