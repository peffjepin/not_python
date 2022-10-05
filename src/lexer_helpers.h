#ifndef LEXER_HELPERS_H
#define LEXER_HELPERS_H

#include <stddef.h>

#include "arena.h"
#include "generated.h"
#include "syntax.h"
#include "tokens.h"

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
    type** prefix##_vector_finalize(type##Vector* vec)

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
    type* prefix##_vector_finalize(type##Vector* vec)

PTR_VECTOR_DECLARATION(Expression, expr);
PTR_VECTOR_DECLARATION(ItGroup, itgroup);
PTR_VECTOR_DECLARATION(Statement, stmt);
VALUE_VECTOR_DECLARATION(SourceString, str);
VALUE_VECTOR_DECLARATION(ItIdentifier, itid);
VALUE_VECTOR_DECLARATION(ElifStatement, elif);
VALUE_VECTOR_DECLARATION(ExceptStatement, except);
VALUE_VECTOR_DECLARATION(TypeInfo, typing);

char* file_namespace(Arena* arena, const char* filepath);

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
#define INDENTATION_MAX_STR "10"

typedef struct {
    size_t count;
    unsigned int values[INDENTATION_MAX];
} IndentationStack;

// in the case of an error returns error message
// else returns NULL
char* indent_check(IndentationStack* stack, Location loc, bool begin_block);

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
void et_push_operation_type(ExpressionTable* et, Operator op_type, Location* loc);
Expression* et_to_expr(ExpressionTable* et);

NpthonType cstr_to_python_type(char* cstr);

#endif
