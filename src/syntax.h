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
typedef struct IfStatement IfStatement;

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

// TODO: requires implementation
typedef struct {
    size_t path_count;
    char** dotted_path;
} ImportPath;

// TODO: requires implementation
typedef struct {
    ImportPath from;
    char* as;
    size_t what_count;
    char** what;
} ImportStatement;

// TODO: requires implementation
typedef struct {
    size_t stmts_count;
    Statement* stmts;
} Block;

// TODO: requires implementation
typedef struct {
    Expression* condition;
    Block body;
} WhileStatement;

// TODO: requires implementation
struct IfStatement {
    Expression* condition;
    Block body;
    size_t elifs_count;
    IfStatement* elifs;
    Block else_body;
};

// TODO: requires implementation
typedef struct {
    Block try_body;
    size_t exceptions_count;
    char** exceptions;
    char* exception_as;
    Block except_body;
    Block finally_body;
    Block else_body;
} TryStatement;

// TODO: requires implementation
typedef struct {
    Expression* ctx_manager;
    char* as;
    Block body;
} WithStatement;

// TODO: requires implementation
typedef struct {
    char* name;
    size_t bases_count;
    char** bases;
    Block body;
} ClassStatement;

// TODO: requires implementation
typedef struct {
    char** names;
    char** types;
    size_t defaults_count;
    Expression** defaults;
} Signature;

// TODO: requires implementation
typedef struct {
    char* name;
    Signature sig;
    Block body;
} FunctionStatement;

// TODO: requires implementation
typedef struct {
    Expression* storage;
    Expression* value;
    Operator op_type;
} AssignementStatement;

typedef struct {
    ItGroup* it;
    Expression* iterable;
    Block body;
} ForLoopStatement;

typedef enum {
    NULL_STMT,
    STMT_EXPR,
    STMT_FOR_LOOP,
    STMT_NO_OP,
    STMT_IMPORT,
    STMT_WHILE,
    STMT_IF,
    STMT_TRY,
    STMT_WITH,
    STMT_CLASS,
    STMT_FUNCTION,
    STMT_ASSIGNMENT,
    STMT_EOF,
} StatementKind;

struct Statement {
    StatementKind kind;
    union {
        ForLoopStatement* for_loop;
        ImportStatement* import_stmt;
        WhileStatement* while_stmt;
        IfStatement* if_stmt;
        TryStatement* try_stmt;
        WithStatement* with_stmt;
        ClassStatement* class_stmt;
        FunctionStatement* function_stmt;
        AssignementStatement* assignment_stmt;
        Expression* expr;
    };
    Location loc;
};

#endif
