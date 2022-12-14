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
typedef struct Symbol Symbol;

typedef enum {
    NPTYPE_UNTYPED,
    NPTYPE_NONE,
    NPTYPE_INT,
    NPTYPE_FLOAT,
    NPTYPE_STRING,
    NPTYPE_LIST,
    NPTYPE_TUPLE,
    NPTYPE_DICT,
    NPTYPE_OBJECT,
    NPTYPE_BOOL,
    NPTYPE_SLICE,
    NPTYPE_ITER,
    NPTYPE_FUNCTION,
    NPTYPE_DICT_ITEMS,
    NPTYPE_CONTEXT,
    NPTYPE_EXCEPTION,
    NPTYPE_POINTER,
    NPTYPE_BYTE,
    NPTYPE_UNSIGNED,
    NPTYPE_CSTR,
    NPTYPE_COUNT,
} NpType;

typedef struct {
    size_t count;
    TypeInfo* types;
} TypeInfoInner;

struct TypeInfo {
    NpType type;
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
    bool is_method;
    Block body;
    Expression* decorator;
    LexicalScope* scope;
} FunctionStatement;

struct ClassStatement {
    SourceString name;
    SourceString base;
    SourceString ns_ident;
    SourceString fmtstr;
    size_t fmtstr_index;
    size_t nbytes;
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

typedef struct {
    Expression* expr;
    SourceString source_code;
} AssertStatement;

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
    STMT_BREAK,
    STMT_CONTINUE,
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
        Expression* return_expr;  // maybe NULL
        AssertStatement* assert_;
        Expression* expr;
    };
    Location loc;
};

// TODO: not sure if the following belongs in this header or not yet.
// it lives here for now

struct Variable {
    enum {
        VAR_REGULAR,
        VAR_SEMI_SCOPED,
        VAR_ARGUMENT,
        VAR_CLOSURE,
        VAR_SELF,
        VAR_CLOSURE_ARGUMENT
    } kind;
    union {
        bool in_scope;          // VAR_SEMI_SCOPED
        size_t closure_offset;  // VAR_CLOSURE / VAR_CLOSURE_ARGUMENT
    };
    SourceString identifier;
    SourceString compiled_name;
    TypeInfo type_info;
};

struct Symbol {
    enum { SYM_VARIABLE, SYM_FUNCTION, SYM_CLASS, SYM_MEMBER, SYM_GLOBAL } kind;
    SourceString identifier;
    union {
        Variable* variable;
        Variable* globalvar;
        FunctionStatement* func;
        ClassStatement* cls;
        TypeInfo* member_type;
    };
};

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
    enum {
        SCOPE_TOP,
        SCOPE_FUNCTION,
        SCOPE_CLOSURE_CHILD,
        SCOPE_CLOSURE_PARENT,
        SCOPE_CLASS
    } kind;
    union {
        FunctionStatement* func;
        ClassStatement* cls;
    };
    SymbolHashmap hm;
};

#endif
