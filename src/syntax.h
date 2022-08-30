#ifndef COMPILER_TYPES_H
#define COMPILER_TYPES_H

#include "generated.h"
#include "tokens.h"

typedef struct Comprehension Comprehension;
typedef struct Enclosure Enclosure;
typedef struct Expression Expression;
typedef struct Arguments Arguments;
typedef struct Slice Slice;
typedef struct Operation Operation;
typedef struct Operand Operand;
typedef struct ItGroup ItGroup;
typedef struct Statement Statement;

typedef enum {
    OPERAND_EXPRESSION,
    OPERAND_ENCLOSURE_LITERAL,
    OPERAND_COMPREHENSION,
    OPERAND_TOKEN,
    OPERAND_ARGUMENTS,
    OPERAND_SLICE,
} OperandKind;

struct Operand {
    OperandKind kind;
    union {
        Token token;
        Comprehension* comp;
        Enclosure* enclosure;
        Expression* expr;
        Arguments* args;
        Slice* slice;
    };
};

struct Operation {
    Operator op_type;
    size_t left;
    size_t right;
};

struct Expression {
    size_t operands_count;
    Operand* operands;
    size_t operations_count;
    Operation* operations;
};

struct Arguments {
    size_t values_count;
    size_t n_positional;
    Expression** values;
    char** kwds;
};

struct Slice {
    Expression* start_expr;
    Expression* stop_expr;
    Expression* step_expr;
};

typedef enum {
    ENCLOSURE_LIST,
    ENCLOSURE_DICT,
    ENCLOSURE_TUPLE,
} EnclosureType;

typedef enum {
    COMPREHENSION_MAPPED,
    COMPREHENSION_SEQUENCE,
} ComprehensionKind;

typedef struct {
    enum { IT_GROUP, IT_ID } kind;
    union {
        ItGroup* group;
        char* name;
    };
} ItIdentifier;

struct ItGroup {
    size_t identifiers_count;
    ItIdentifier* identifiers;
};

typedef struct {
    size_t loop_count;
    ItGroup** its;
    Expression** iterables;
    Expression* if_expr;
} ComprehensionBody;

typedef struct {
    Expression* key_expr;
    Expression* val_expr;
} MappedComprehension;

typedef struct {
    Expression* expr;
} SequenceComprehension;

struct Comprehension {
    EnclosureType type;
    ComprehensionBody body;
    union {
        MappedComprehension mapped;
        SequenceComprehension sequence;
    };
};

struct Enclosure {
    EnclosureType type;
    size_t expressions_count;
    Expression** expressions;
};

typedef enum {
    NULL_STMT,
    STMT_EXPR,
    STMT_FOR_LOOP,
    STMT_NO_OP,
    STMT_EOF,
} StatementKind;

typedef struct {
    ItGroup* it;
    Expression* iterable;
    size_t body_length;
    Statement* body;
} ForLoopStatement;

struct Statement {
    StatementKind kind;
    union {
        ForLoopStatement* for_loop;
        Expression* expr;
    };
    Location loc;
};

#endif
