#ifndef COMPILER_TYPES_H
#define COMPILER_TYPES_H

#include "arena.h"
#include "generated.h"
#include "object_model.h"
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
typedef struct Variable Variable;
typedef struct ClassStatement ClassStatement;
typedef struct Signature Signature;

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
    PYTYPE_SLICE,
    PYTYPE_ITER,
    PYTYPE_FUNCTION,
    PYTYPE_DICT_ITEMS,
    PYTYPE_COUNT,
} PythonType;

typedef struct {
    size_t count;
    TypeInfo* types;
} TypeInfoInner;

struct TypeInfo {
    PythonType type;
    union {
        ClassStatement* cls;
        // NOTE: sig->defaults and sig->params may be NULL
        // when accessing a signature from TypeInfo because
        // it may derive from a type hint
        Signature* sig;
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
    Location* loc;
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
    SourceString* kwds;
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
        SourceString name;
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
    SourceString* dotted_path;
} ImportPath;

typedef struct {
    ImportPath from;
    size_t what_count;
    SourceString* what;
    SourceString* as;
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
    SourceString* exceptions;
    SourceString as;
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
    SourceString as;
    Block body;
} WithStatement;

struct Signature {
    size_t params_count;
    SourceString* params;
    TypeInfo* types;
    size_t defaults_count;
    Expression** defaults;
    TypeInfo return_type;
};

typedef struct {
    SourceString name;
    SourceString ns_ident;
    Signature sig;
    Block body;
    LexicalScope* scope;
} FunctionStatement;

struct ClassStatement {
    SourceString name;
    SourceString base;
    SourceString ns_ident;
    SourceString fmtstr;
    Signature sig;
    Block body;
    LexicalScope* scope;
    FunctionStatement* object_model_methods[OBJECT_MODEL_COUNT];
};

typedef struct {
    Expression* storage;
    Expression* value;
    Operator op_type;
} AssignmentStatement;

typedef struct {
    SourceString identifier;
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
    STMT_RETURN,
    STMT_ASSERT,
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
        Expression* return_expr;
        Expression* assert_expr;
        Expression* expr;
    };
    Location loc;
};

// TODO: not sure if the following belongs in this header or not yet.
// it lives here for now

struct Variable {
    SourceString identifier;
    SourceString ns_ident;
    TypeInfo type;
    bool declared;
};

// for k, v in d1.items():
//     ...
// print(k, v)  # k, v still in scope
// for k, v in d2.items():
//     ...
// print(k, v)  # k, v can be reused and have their types changed
//
// This struct is to allow for this behavior while a standard variable
// is defined only once and when it's type is determined it is immutable.
typedef struct {
    SourceString identifier;
    SourceString current_id;
    TypeInfo type;
    bool directly_in_scope;  // type is immutable while directly in scope
} SemiScopedVariable;

typedef struct {
    enum { SYM_VARIABLE, SYM_SEMI_SCOPED, SYM_FUNCTION, SYM_CLASS } kind;
    union {
        Variable* variable;
        SemiScopedVariable* semi_scoped;
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
Symbol* symbol_hm_get(SymbolHashmap* hm, SourceString identifier);
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
