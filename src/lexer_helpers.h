#ifndef LEXER_HELPERS_H
#define LEXER_HELPERS_H

#include <stddef.h>

#include "arena.h"
#include "generated.h"
#include "syntax.h"
#include "tokens.h"

#define UNIMPLEMENTED(msg) assert(0 && msg)

#define SYNTAX_ERROR(loc, msg)                                                           \
    do {                                                                                 \
        fprintf(                                                                         \
            stderr,                                                                      \
            "%s:%u:%u SYNTAX ERROR: %s\n",                                               \
            (loc).filename,                                                              \
            (loc).line,                                                                  \
            (loc).col,                                                                   \
            msg                                                                          \
        );                                                                               \
        exit(1);                                                                         \
    } while (0)

#define SYNTAX_ERRORF(loc, msg, ...)                                                     \
    do {                                                                                 \
        fprintf(                                                                         \
            stderr,                                                                      \
            "%s:%u:%u SYNTAX ERROR: " msg "\n",                                          \
            (loc).filename,                                                              \
            (loc).line,                                                                  \
            (loc).col,                                                                   \
            __VA_ARGS__                                                                  \
        );                                                                               \
        exit(1);                                                                         \
    } while (0)

/*
 * Notes on vector declarations
 *
 * Vectors defined with these macros are allocated in dynamic arena memory
 * and are meant to be finalized (allocated into static arena memory) once
 * their size has been determined.
 *
 * Vectors declared with PTR_VECTOR_DECLARATION take their elements by pointers
 * and when finalized return type**;
 *
 * Vectors declared with VALUE_VECTOR_DECLARATION take their elements by value
 * and when finalized return type*
 */

#define PTR_VECTOR_DECLARATION(type, prefix)                                             \
    typedef struct {                                                                     \
        Arena* arena;                                                                    \
        type** data;                                                                     \
        size_t capacity;                                                                 \
        size_t count;                                                                    \
        size_t bytes;                                                                    \
    } type##Vector;                                                                      \
    type##Vector prefix##_vector_init(Arena* arena);                                     \
    void prefix##_vector_append(type##Vector* vec, type* ptr);                           \
    type** prefix##_vector_finalize(type##Vector* vec);

#define VALUE_VECTOR_DECLARATION(type, prefix)                                           \
    typedef struct {                                                                     \
        Arena* arena;                                                                    \
        type* data;                                                                      \
        size_t capacity;                                                                 \
        size_t count;                                                                    \
        size_t bytes;                                                                    \
    } type##Vector;                                                                      \
    type##Vector prefix##_vector_init(Arena* arena);                                     \
    void prefix##_vector_append(type##Vector* vec, type value);                          \
    type* prefix##_vector_finalize(type##Vector* vec);

// kind of a hack to define StringVector without having to typedef String
#define String char
PTR_VECTOR_DECLARATION(String, str)
#undef String

PTR_VECTOR_DECLARATION(Expression, expr)
PTR_VECTOR_DECLARATION(ItGroup, itgroup)
VALUE_VECTOR_DECLARATION(ItIdentifier, itid)
VALUE_VECTOR_DECLARATION(Statement, stmt)
VALUE_VECTOR_DECLARATION(ElifStatement, elif)
VALUE_VECTOR_DECLARATION(ExceptStatement, except)

void out_of_memory(void);

#define TOKEN_QUEUE_CAPACITY 8

const char* token_type_to_cstr(TokenType type);

typedef struct {
    size_t head;
    size_t length;
    Token tokens[TOKEN_QUEUE_CAPACITY];
} TokenQueue;

Token tq_peek(TokenQueue* tq, size_t offset);
Token tq_consume(TokenQueue* tq);
void tq_push(TokenQueue* tq, Token token);
// cheaper option to consume the next item after peeking
void tq_discard(TokenQueue* tq);

// at the moment I'm not supporting nested classes or functions
// so the most we would have is 3: top_level -> class -> function
#define SCOPE_STACK_MAX 3
LexicalScope* scope_init(Arena* arena);

typedef struct {
    size_t count;
    LexicalScope* scopes[SCOPE_STACK_MAX];
} LexicalScopeStack;

LexicalScope* scope_stack_peek(LexicalScopeStack* stack);
void scope_stack_push(LexicalScopeStack* stack, LexicalScope* scope);
LexicalScope* scope_stack_pop(LexicalScopeStack* stack);

#define INDENTATION_MAX 10

typedef struct {
    size_t count;
    unsigned int values[INDENTATION_MAX];
} IndentationStack;

void indent_check(IndentationStack* stack, Location loc, bool begin_block);

typedef struct {
    size_t count;
    size_t capacity;
    Operation* operations;
} OperationVector;

void operation_vector_push(OperationVector* vec, Operation operation);

typedef struct {
    Arena* arena;
    enum { ET_NONE, ET_OPERAND, ET_OPERATION } previous;
    size_t operands_count;
    size_t operands_capacity;
    size_t operands_nbytes;
    Operand* operands;
    size_t operations_count;
    OperationVector operation_vectors[MAX_PRECEDENCE + 1];
} ExpressionTable;

ExpressionTable et_init(Arena* arena);
void et_push_operand(ExpressionTable* et, Operand operand);
void et_push_operation(ExpressionTable* et, Operation operation);
void et_push_operation_type(ExpressionTable* et, Operator op_type);
Expression* et_to_expr(ExpressionTable* et);

size_t filename_offset(const char* filepath);

#endif
