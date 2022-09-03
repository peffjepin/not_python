#ifndef COMPILER_TYPES_H
#define COMPILER_TYPES_H

#include "arena.h"
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
typedef struct LexicalScope LexicalScope;
typedef struct TypeInfo TypeInfo;

typedef enum {
    PYTYPE_UNTYPED,
    PYTYPE_NONE,
    PYTYPE_INT,
    PYTYPE_FLOAT,
    PYTYPE_STRING,
    PYTYPE_LIST,
    PYTYPE_TUPLE,
    PYTYPE_DICT,
    PYTYPE_OBJECT,
    PYTYPE_BOOL,
} PythonType;

typedef struct {
    size_t count;
    TypeInfo* types;
} TypeInfoInner;

struct TypeInfo {
    PythonType type;
    union {
        char* class_name;
        TypeInfoInner* inner;
    };
};

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

typedef struct {
    size_t path_count;
    char** dotted_path;
} ImportPath;

typedef struct {
    ImportPath from;
    size_t what_count;
    char** what;
    char** as;
} ImportStatement;

typedef struct {
    size_t stmts_count;
    Statement** stmts;
} Block;

typedef struct {
    Expression* condition;
    Block body;
} WhileStatement;

typedef struct {
    Expression* condition;
    Block body;
} ElifStatement;

typedef struct {
    Expression* condition;
    Block body;
    size_t elifs_count;
    ElifStatement* elifs;
    Block else_body;
} ConditionalStatement;

typedef struct {
    size_t exceptions_count;
    char** exceptions;
    char* as;
    Block body;
} ExceptStatement;

typedef struct {
    Block try_body;
    size_t excepts_count;
    ExceptStatement* excepts;
    Block finally_body;
    Block else_body;
} TryStatement;

typedef struct {
    Expression* ctx_manager;
    char* as;
    Block body;
} WithStatement;

typedef struct {
    char* name;
    char* base;
    Block body;
    LexicalScope* scope;
} ClassStatement;

typedef struct {
    size_t params_count;
    char** params;
    TypeInfo* types;
    size_t defaults_count;
    Expression** defaults;
    TypeInfo return_type;
} Signature;

typedef struct {
    char* name;
    Signature sig;
    Block body;
    LexicalScope* scope;
} FunctionStatement;

typedef struct {
    Expression* storage;
    Expression* value;
    Operator op_type;
} AssignmentStatement;

typedef struct {
    char* identifier;
    TypeInfo type;
    Expression* initial;
} AnnotationStatement;

// techincally this could have an else at the end
// but I'm not sure I care to support that
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
    STMT_ANNOTATION,
    STMT_EOF,
} StatementKind;

struct Statement {
    StatementKind kind;
    union {
        ForLoopStatement* for_loop;
        ImportStatement* import;
        WhileStatement* while_loop;
        ConditionalStatement* conditional;
        TryStatement* try_stmt;
        WithStatement* with;
        ClassStatement* cls;
        FunctionStatement* func;
        AssignmentStatement* assignment;
        AnnotationStatement* annotation;
        Expression* expr;
    };
    Location loc;
};

// TODO: not sure if the following belongs in this header or not yet.
// it lives here for now

typedef struct {
    char* identifier;
    TypeInfo type;
} Variable;

typedef struct {
    enum { SYM_VARIABLE, SYM_PARAM, SYM_MEMBER, SYM_FUNCTION, SYM_CLASS } kind;
    AssignmentStatement* first_assignment;  // NULL when not applicable
    union {
        Variable* variable;
        Variable* member;
        FunctionStatement* func;
        ClassStatement* cls;
    };
} Symbol;

typedef struct {
    bool finalized;
    Arena* arena;
    size_t bytes;
    uint8_t* buffer;
    size_t elements_capacity;
    size_t elements_count;
    Symbol* elements;
    size_t lookup_capacity;
    int* lookup_table;
} SymbolHashmap;

SymbolHashmap symbol_hm_init(Arena* arena);
void symbol_hm_put(SymbolHashmap* hm, Symbol element);
Symbol* symbol_hm_get(SymbolHashmap* hm, char* identifier);
void symbol_hm_finalize(SymbolHashmap* hm);

struct LexicalScope {
    enum { SCOPE_TOP, SCOPE_FUNCTION, SCOPE_METHOD, SCOPE_CLASS } kind;
    union {
        FunctionStatement* func;
        ClassStatement* cls;
    };
    SymbolHashmap hm;
};

#endif
