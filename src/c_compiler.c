#include "c_compiler.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "c_compiler_helpers.h"
#include "diagnostics.h"
#include "lexer_helpers.h"
#include "not_python.h"
#include "np_hash.h"
#include "object_model.h"
#include "syntax.h"
#include "type_checker.h"

#define STRING_CONSTANTS_TABLE_NAME "NOT_PYTHON_STRING_CONSTANTS"

#define NPLIB_PRINT "builtin_print"

#define NPLIB_ALLOC "np_alloc"

#define NPLIB_LIST_APPEND "np_list_append"
#define NPLIB_LIST_CLEAR "np_list_clear"
#define NPLIB_LIST_COUNT "np_list_count"
#define NPLIB_LIST_EXTEND "np_list_extend"
#define NPLIB_LIST_INDEX "np_list_index"
#define NPLIB_LIST_INSERT "np_list_insert"
#define NPLIB_LIST_POP "np_list_pop"
#define NPLIB_LIST_REMOVE "np_list_remove"
#define NPLIB_LIST_REVERSE "np_list_reverse"
#define NPLIB_LIST_SORT "np_list_sort"
#define NPLIB_LIST_COPY "np_list_copy"
#define NPLIB_LIST_GET_ITEM "np_list_get_item"
#define NPLIB_LIST_SET_ITEM "np_list_set_item"
#define NPLIB_LIST_ADD "np_list_add"
#define NPLIB_LIST_INIT "LIST_INIT"
#define NPLIB_LIST_ITER "np_list_iter"

#define NPLIB_DICT_CLEAR "np_dict_clear"
#define NPLIB_DICT_COPY "np_dict_copy"
#define NPLIB_DICT_GET "np_dict_get"
#define NPLIB_DICT_ITEMS "np_dict_iter_items"
#define NPLIB_DICT_KEYS "np_dict_iter_keys"
#define NPLIB_DICT_VALUES "np_dict_iter_vals"
#define NPLIB_DICT_POP "np_dict_pop_val"
#define NPLIB_DICT_POPITEM "np_dict_popitem"
#define NPLIB_DICT_UPDATE "np_dict_update"
#define NPLIB_DICT_GET_ITEM "np_dict_get_val"
#define NPLIB_DICT_INIT "DICT_INIT"
#define NPLIB_DICT_SET_ITEM "np_dict_set_item"

#define NPLIB_STR_ADD "np_str_add"
#define NPLIB_STR_EQ "np_str_eq"
#define NPLIB_STR_GT "np_str_gt"
#define NPLIB_STR_GTE "np_str_gte"
#define NPLIB_STR_LT "np_str_lt"
#define NPLIB_STR_LTE "np_str_lte"
#define NPLIB_STR_TO_CSTR "np_str_to_cstr"
#define NPLIB_STR_FMT "np_str_fmt"

#define NPLIB_CURRENT_EXCEPTS_CTYPE "uint64_t"
#define NPLIB_CURRENT_EXCEPTS "current_excepts"
#define NPLIB_GET_EXCEPTION "get_exception"
#define NPLIB_ASSERTION_ERROR "assertion_error"

#define NPLIB_FMOD "fmod"
#define NPLIB_POW "pow"

#define UNTYPED                                                                          \
    (TypeInfo) { .type = NPTYPE_UNTYPED }
#define NONE_TYPE                                                                        \
    (TypeInfo) { .type = NPTYPE_NONE }
#define INT_TYPE                                                                         \
    (TypeInfo) { .type = NPTYPE_INT }
#define FLOAT_TYPE                                                                       \
    (TypeInfo) { .type = NPTYPE_FLOAT }
#define STRING_TYPE                                                                      \
    (TypeInfo) { .type = NPTYPE_STRING }
#define BOOL_TYPE                                                                        \
    (TypeInfo) { .type = NPTYPE_BOOL }
#define EXCEPTION_TYPE                                                                   \
    (TypeInfo) { .type = NPTYPE_EXCEPTION }

typedef struct Instruction Instruction;

typedef struct {
    Arena* arena;
    size_t count;
    size_t capacity;
    size_t bytes;
    Instruction* instructions;
} CompoundInstruction;

typedef struct {
    enum { IDENT_CSTR, IDENT_VAR } kind;
    union {
        const char* cstr;
        Variable* var;
    };
} Ident;

#define NULL_IDENT                                                                       \
    (Ident) { 0 }

static const char*
ident(Ident i)
{
    switch (i.kind) {
        case IDENT_CSTR:
            return i.cstr;
        case IDENT_VAR:
            return i.var->compiled_name.data;
    }
    UNREACHABLE();
}

// TODO: eventually migrate these data structures towards being backend agnostic so
// compilation can be accomplished with various backends with the same data structures

typedef struct {
    Ident left;
    const char* right;
    TypeInfo type_info;
} AssignmentInst;

typedef struct {
    Ident ident;
    TypeInfo type_info;
    const char* ctype;  // fallback only if type_info.type == NPTYPE_UNTYPED
} DeclareVariableInst;

typedef struct {
    const char* condition;
    const char* after_label;
    CompoundInstruction init;
    CompoundInstruction before;
    CompoundInstruction body;
    CompoundInstruction after;
} LoopInst;

typedef struct {
    const char* function_name;
    Signature signature;
    CompoundInstruction body;
    DeclareVariableInst public_variable;
    AssignmentInst assign_fn_variable;
} DefineFunctionInst;

typedef struct {
    const char* class_name;
    Signature signature;
    CompoundInstruction body;
} DefineClassInst;

typedef struct {
    bool negate;
    AssignmentInst condition_assignment;
    CompoundInstruction body;
} IfInst;

struct Instruction {
    enum {
        INST_NO_OP,
        INST_ASSIGNMENT,
        INST_EXPRESSION,
        INST_LOOP,
        INST_DECLARE_VARIABLE,
        INST_DEFINE_FUNCTION,
        INST_DEFINE_CLASS,
        INST_IF,
        INST_ELSE,
        INST_GOTO,
        INST_LABEL,
        INST_BREAK,
        INST_CONTINUE,
    } kind;
    union {
        AssignmentInst assignment;
        const char* expression;
        LoopInst loop;
        DeclareVariableInst declare_variable;
        DefineFunctionInst define_function;
        DefineClassInst define_class;
        IfInst if_;
        CompoundInstruction else_;
        const char* label;
    };
};

#define NO_OP                                                                            \
    (Instruction) { 0 }

static const char*
inst_ident(Instruction inst)
{
    switch (inst.kind) {
        case INST_ASSIGNMENT:
            return ident(inst.assignment.left);
        case INST_DECLARE_VARIABLE:
            return ident(inst.declare_variable.ident);
        default:
            UNIMPLEMENTED("extracting an identifier unimplemented for instruction type");
    }
}

typedef struct {
    Arena* arena;
    Requirements reqs;
    LexicalScope* top_level_scope;
    LexicalScopeStack scope_stack;
    StringHashmap str_hm;
    FileIndex file_index;
    Location current_stmt_location;
    Location current_operation_location;
    TypeChecker tc;
    CompoundInstruction instructions;
    Instruction previous_instruction;
    size_t unique_vars_counter;
    const char* excepts_goto;
    const char* loop_after;
    StringBuilder sb;
} C_Compiler;

static void compile_statement(C_Compiler* compiler, Statement* stmt);
static Instruction expr_to_assignment_inst(
    C_Compiler* compiler, TypeInfo type_info, Ident dest, Expression* expr
);
static Instruction convert_assignment_to_string(
    C_Compiler* compiler, AssignmentInst assignment_to_convert
);
static const char* assignment_to_truthy(C_Compiler* compiler, AssignmentInst assignment);
static void declare_scope_variables(C_Compiler* compiler, LexicalScope* scope);

CompoundInstruction
compound_instruction_init(Arena* arena)
{
    CompoundInstruction inst = {.arena = arena};
    inst.arena = arena;
    inst.instructions = arena_dynamic_alloc(arena, &inst.bytes);
    inst.capacity = inst.bytes / sizeof(Instruction);
    return inst;
}

void
compound_instruction_append(CompoundInstruction* compound, Instruction inst)
{
    if (compound->count == compound->capacity) {
        compound->instructions =
            arena_dynamic_grow(compound->arena, compound->instructions, &compound->bytes);
        compound->capacity = compound->bytes / sizeof(Instruction);
    }
    compound->instructions[compound->count++] = inst;
}

void
compound_instruction_finalize(CompoundInstruction* compound)
{
    compound->instructions =
        arena_dynamic_finalize(compound->arena, compound->instructions, compound->bytes);
}

Instruction*
add_instruction(C_Compiler* compiler, Instruction inst)
{
    if (inst.kind == INST_DECLARE_VARIABLE &&
        inst.declare_variable.ident.kind == IDENT_CSTR &&
        inst.declare_variable.type_info.type == NPTYPE_UNTYPED &&
        !inst.declare_variable.ctype) {
        UNTYPED_ERROR();
    }
    compiler->previous_instruction = inst;
    compound_instruction_append(&compiler->instructions, inst);
    return compiler->instructions.instructions + (compiler->instructions.count - 1);
}

static const char*
generate_ident(C_Compiler* compiler)
{
    char* buf = arena_alloc(compiler->arena, 16);
    snprintf(buf, 16, "_np_%zu", compiler->unique_vars_counter++);
    return (const char*)buf;
}

// Should be called at most once for each assignment.
// Sets the type info for associated Variable struct when applicable
// Enforces existing type info from Variable struct when applicable
// Errors if type is already set
static void
set_assignment_type_info(
    C_Compiler* compiler, AssignmentInst* assignment, TypeInfo type_info
)
{
    assert(type_info.type != NPTYPE_UNTYPED && "trying to set type info to untyped");

    if (assignment->type_info.type != NPTYPE_UNTYPED)
        error("trying to set the type on an AssignmentInst multiple times");

    assignment->type_info = type_info;

    switch (assignment->left.kind) {
        case IDENT_CSTR:
            break;
        case IDENT_VAR: {
            Variable* var = assignment->left.var;
            if (assignment->left.var->type_info.type == NPTYPE_UNTYPED) {
                var->type_info = type_info;
            }
            else if (!compare_types(var->type_info, type_info)) {
                // TODO: location might not be accurate here
                type_errorf(
                    compiler->file_index,
                    compiler->current_operation_location,
                    "variable `%s` of previously defined type `%s` trying to have type "
                    "`%s` assigned",
                    var->identifier.data,
                    errfmt_type_info(var->type_info),
                    errfmt_type_info(type_info)
                );
            }
        } break;
    }
}

// Can be called safely multiple times on an AssignmentInst
// Enforces existing type info
// Sets type info if it's not already set
static void
check_assignment_type_info(
    C_Compiler* compiler, AssignmentInst* assignment, TypeInfo type_info
)
{
    assert(type_info.type != NPTYPE_UNTYPED && "trying to set type info to untyped");

    if (assignment->type_info.type == NPTYPE_UNTYPED)
        set_assignment_type_info(compiler, assignment, type_info);
    else if (!compare_types(assignment->type_info, type_info))
        // TODO: location might not be accurate here
        type_errorf(
            compiler->file_index,
            compiler->current_operation_location,
            "expecting type `%s` but got type `%s`",
            errfmt_type_info(assignment->type_info),
            errfmt_type_info(type_info)
        );
}

static Symbol*
get_symbol(C_Compiler* compiler, SourceString identifier)
{
    // TODO: need to treat globals/locals differently in the furute
    return get_symbol_from_scopes(compiler->scope_stack, identifier);
}

static const char*
token_operand_repr(C_Compiler* compiler, Operand operand)
{
    if (operand.token.type == TOK_IDENTIFIER) {
        // TODO: None should probably be a keyword
        static const SourceString NONE_STR = {.data = "None", .length = 4};
        if (SOURCESTRING_EQ(operand.token.value, NONE_STR)) return "NULL";
        // TODO: should consider a way to avoid this lookup because it happens often
        Symbol* sym = get_symbol(compiler, operand.token.value);
        switch (sym->kind) {
            case SYM_GLOBAL:
                return sym->globalvar->compiled_name.data;
            case SYM_VARIABLE:
                return sym->variable->compiled_name.data;
            case SYM_FUNCTION:
                return sym->func->ns_ident.data;
            case SYM_CLASS:
                return sym->cls->ns_ident.data;
            case SYM_MEMBER:
                UNREACHABLE();
        }
        UNREACHABLE();
    }
    else if (operand.token.type == TOK_NUMBER) {
        return operand.token.value.data;
    }
    else if (operand.token.type == TOK_STRING) {
        size_t index = str_hm_put(&compiler->str_hm, operand.token.value);
        char strindex[10];
        sprintf(strindex, "%zu", index);
        SourceString str = sb_build(
            &compiler->sb, STRING_CONSTANTS_TABLE_NAME, "[", strindex, "]", NULL
        );
        return str.data;
    }
    else if (operand.token.type == TOK_KEYWORD) {
        if (operand.token.kw == KW_TRUE)
            return "true";
        else if (operand.token.kw == KW_FALSE)
            return "false";
        else
            UNREACHABLE();
    }
    else
        UNREACHABLE();
}

// ... should be NULL terminated sequence of (const char*) arguments
static const char*
c_function_call(C_Compiler* compiler, const char* fn_name, ...)
{
    sb_begin(&compiler->sb);

    sb_add(&compiler->sb, fn_name);
    sb_add(&compiler->sb, "(");

    va_list args;
    va_start(args, fn_name);
    const char* s = va_arg(args, const char*);
    size_t argc = 0;
    while (s) {
        if (argc++ > 0) sb_add(&compiler->sb, ", ");
        sb_add(&compiler->sb, s);
        s = va_arg(args, const char*);
    }
    va_end(args);

    sb_add(&compiler->sb, ")");

    return sb_end(&compiler->sb);
}

// ... should be NULL terminated sequence of (const char*) arguments
static const char*
c_function_calln(
    C_Compiler* compiler, const char* fn_name, const char** args, size_t argc
)
{
    sb_begin(&compiler->sb);

    sb_add(&compiler->sb, fn_name);
    sb_add(&compiler->sb, "(");

    for (size_t i = 0; i < argc; i++) {
        if (i > 0) sb_add(&compiler->sb, ", ");
        sb_add(&compiler->sb, args[i]);
    }

    sb_add(&compiler->sb, ")");

    return sb_end(&compiler->sb);
}

// -1 on bad kwd
// TODO: should check if parser even allows this to happen in the future
static int
index_of_kwarg(Signature sig, SourceString kwd)
{
    for (size_t i = 0; i < sig.params_count; i++) {
        if (SOURCESTRING_EQ(kwd, sig.params[i])) return i;
    }
    return -1;
}

static const char* NPLIB_CONVERSION_FUNCTIONS[NPTYPE_COUNT][NPTYPE_COUNT] = {
    [NPTYPE_INT] =
        {
            [NPTYPE_STRING] = "np_int_to_str",
        },
    [NPTYPE_FLOAT] =
        {
            [NPTYPE_STRING] = "np_float_to_str",
        },
    [NPTYPE_BOOL] =
        {
            [NPTYPE_STRING] = "np_bool_to_str",
        },
};

#define COMPILER_STRING(compiler, ...) sb_build(&compiler->sb, __VA_ARGS__, NULL).data

static Instruction
default_object_string_representation(
    C_Compiler* compiler, ClassStatement* clsdef, AssignmentInst object_assignment
)
{
    // create a python string that looks like:
    //  ClassName(param1=param1_value, param2=param2_value...)
    if (!clsdef->fmtstr.data)
        // default fmt string expects all params to be first convereted into cstr
        clsdef->fmtstr = create_default_object_fmt_str(compiler->arena, clsdef);

    size_t argc = clsdef->sig.params_count + 1;
    const char* arg_idents[argc];
    arg_idents[0] = COMPILER_STRING(compiler, "\"", clsdef->fmtstr.data, "\"");

    for (size_t i = 1; i < argc; i++) {
        Instruction conversion_inst = convert_assignment_to_string(
            compiler,
            (AssignmentInst){
                .left.cstr = COMPILER_STRING(
                    compiler,
                    ident(object_assignment.left),
                    "->",
                    clsdef->sig.params[i - 1].data
                ),
                .type_info = clsdef->sig.types[i - 1],
            }
        );
        add_instruction(compiler, conversion_inst);
        arg_idents[i] = c_function_call(
            compiler, NPLIB_STR_TO_CSTR, inst_ident(conversion_inst), NULL
        );
    }

    const char* ident = generate_ident(compiler);
    add_instruction(
        compiler,
        (Instruction){
            .kind = INST_DECLARE_VARIABLE,
            .declare_variable.type_info = STRING_TYPE,
            .declare_variable.ident.cstr = ident}
    );
    return (Instruction){
        .kind = INST_ASSIGNMENT,
        .assignment.left.cstr = ident,
        .assignment.right = c_function_calln(compiler, NPLIB_STR_FMT, arg_idents, argc)};
}

static Instruction
convert_assignment_to_string(C_Compiler* compiler, AssignmentInst assignment_to_convert)
{
    const char* fn_call = NULL;

    switch (assignment_to_convert.type_info.type) {
        case NPTYPE_INT:
            fn_call = c_function_call(
                compiler,
                NPLIB_CONVERSION_FUNCTIONS[NPTYPE_INT][NPTYPE_STRING],
                ident(assignment_to_convert.left),
                NULL
            );
            break;
        case NPTYPE_FLOAT:
            fn_call = c_function_call(
                compiler,
                NPLIB_CONVERSION_FUNCTIONS[NPTYPE_FLOAT][NPTYPE_STRING],
                ident(assignment_to_convert.left),
                NULL
            );
            break;
        case NPTYPE_BOOL:
            fn_call = c_function_call(
                compiler,
                NPLIB_CONVERSION_FUNCTIONS[NPTYPE_BOOL][NPTYPE_STRING],
                ident(assignment_to_convert.left),
                NULL
            );
            break;
        case NPTYPE_OBJECT: {
            FunctionStatement* user_defined_str =
                assignment_to_convert.type_info.cls
                    ->object_model_methods[OBJECT_MODEL_STR];
            if (user_defined_str) {
                fn_call = c_function_call(
                    compiler,
                    user_defined_str->ns_ident.data,
                    ident(assignment_to_convert.left),
                    NULL
                );
            }
            else
                return default_object_string_representation(
                    compiler, assignment_to_convert.type_info.cls, assignment_to_convert
                );
            break;
        }
        default: {
            unspecified_errorf(
                compiler->file_index,
                compiler->current_stmt_location,
                "type conversion from `%s` to `str` not currently implemented",
                errfmt_type_info(assignment_to_convert.type_info)
            );
        }
    }
    const char* id = generate_ident(compiler);
    add_instruction(
        compiler,
        (Instruction){
            .kind = INST_DECLARE_VARIABLE,
            .declare_variable.type_info = STRING_TYPE,
            .declare_variable.ident.cstr = id,
        }
    );
    return (Instruction){
        .kind = INST_ASSIGNMENT,
        .assignment.left.cstr = id,
        .assignment.right = fn_call,
        .assignment.type_info.type = NPTYPE_STRING,
    };
}

static void
render_builtin_print(C_Compiler* compiler, AssignmentInst* assignment, Arguments* args)
{
    if (args->values_count != args->n_positional) {
        type_error(
            compiler->file_index,
            compiler->current_operation_location,
            "print keyword arguments not currently implemented"
        );
    }

    size_t args_count = (args->values_count == 0) ? 2 : args->values_count + 1;
    const char* arg_idents[args_count];
    arg_idents[0] = arena_nfmt(compiler->arena, 10, "%zu", args_count - 1);
    if (args->values_count == 0)
        arg_idents[1] = "(NpString){.data=\"\", .length=0}";
    else {
        for (size_t i = 0; i < args->values_count; i++) {
            add_instruction(
                compiler,
                expr_to_assignment_inst(compiler, UNTYPED, NULL_IDENT, args->values[i])
            );
            if (compiler->previous_instruction.assignment.type_info.type !=
                NPTYPE_STRING) {
                add_instruction(
                    compiler,
                    convert_assignment_to_string(
                        compiler, compiler->previous_instruction.assignment
                    )
                );
            }
            arg_idents[1 + i] = ident(compiler->previous_instruction.assignment.left);
        }
    }

    check_assignment_type_info(compiler, assignment, (TypeInfo){.type = NPTYPE_NONE});
    assignment->right = c_function_calln(compiler, NPLIB_PRINT, arg_idents, args_count);
}

static void
expect_arg_count(C_Compiler* compiler, char* fn_name, Arguments* args, size_t count)
{
    if (args->values_count != count) {
        type_errorf(
            compiler->file_index,
            compiler->current_operation_location,
            "`%s` expecting %zu arguments but got %zu",
            fn_name,
            count,
            args->values_count
        );
    }
}

#define REFERENCE(compiler, cstr_ident)                                                  \
    sb_build(&compiler->sb, "&", cstr_ident, NULL).data
#define REFERENCE_ASSIGNMENT(compiler, assignment_inst)                                  \
    REFERENCE(compiler, ident(assignment_inst.left))
#define REFERENCE_PREVIOUS_ASSIGNMENT(compiler)                                          \
    REFERENCE(compiler, ident(compiler->previous_instruction.assignment.left))
#define REFERENCE_PREVIOUS_INSTRUCTION(compiler)                                         \
    REFERENCE(compiler, inst_ident(compiler->previous_instruction))
#define REFERENCE_INSTRUCTION(compiler, inst) REFERENCE(compiler, inst_ident(inst))

static void
render_list_builtin(
    C_Compiler* compiler,
    AssignmentInst* assignment,
    AssignmentInst list_assignment,
    const char* fn_name,
    Arguments* args
)
{
    assert(fn_name && "fn_name cannot be NULL");
    TypeInfo list_content_type = list_assignment.type_info.inner->types[0];

    // TODO: stop using out variable in library functions

    // TODO: the function objects could probably now handle making references
    // to these calls and calling them through the standard mechanisms while
    // this logic could shift into creating the function objects
    switch (fn_name[0]) {
        case 'a':
            if (strcmp(fn_name, "append") == 0) {
                expect_arg_count(compiler, "list.append", args, 1);
                add_instruction(
                    compiler,
                    expr_to_assignment_inst(
                        compiler, list_content_type, NULL_IDENT, args->values[0]
                    )
                );
                check_assignment_type_info(compiler, assignment, NONE_TYPE);
                assignment->right = c_function_call(
                    compiler,
                    NPLIB_LIST_APPEND,
                    ident(list_assignment.left),
                    REFERENCE_PREVIOUS_ASSIGNMENT(compiler),
                    NULL
                );
                return;
            }
            break;
        case 'c':
            if (strcmp(fn_name, "clear") == 0) {
                expect_arg_count(compiler, "list.clear", args, 0);
                check_assignment_type_info(compiler, assignment, NONE_TYPE);
                assignment->right = c_function_call(
                    compiler, NPLIB_LIST_CLEAR, ident(list_assignment.left), NULL
                );
                return;
            }
            else if (strcmp(fn_name, "copy") == 0) {
                expect_arg_count(compiler, "list.copy", args, 0);
                check_assignment_type_info(
                    compiler, assignment, list_assignment.type_info
                );
                assignment->right = c_function_call(
                    compiler, NPLIB_LIST_COPY, ident(list_assignment.left), NULL
                );
                return;
            }
            else if (strcmp(fn_name, "count") == 0) {
                expect_arg_count(compiler, "list.count", args, 1);
                add_instruction(
                    compiler,
                    expr_to_assignment_inst(
                        compiler, list_content_type, NULL_IDENT, args->values[0]
                    )
                );
                check_assignment_type_info(compiler, assignment, INT_TYPE);
                assignment->right = c_function_call(
                    compiler,
                    NPLIB_LIST_COUNT,
                    ident(list_assignment.left),
                    REFERENCE_PREVIOUS_ASSIGNMENT(compiler),
                    NULL
                );
                return;
            }
            break;
        case 'e':
            if (strcmp(fn_name, "extend") == 0) {
                expect_arg_count(compiler, "list.extend", args, 1);
                add_instruction(
                    compiler,
                    expr_to_assignment_inst(
                        compiler, list_assignment.type_info, NULL_IDENT, args->values[0]
                    )
                );
                check_assignment_type_info(compiler, assignment, NONE_TYPE);
                assignment->right = c_function_call(
                    compiler,
                    NPLIB_LIST_EXTEND,
                    ident(list_assignment.left),
                    ident(compiler->previous_instruction.assignment.left),
                    NULL
                );
                return;
            }
            break;
        case 'i':
            if (strcmp(fn_name, "index") == 0) {
                // TODO: start/stop
                expect_arg_count(compiler, "list.index", args, 1);
                add_instruction(
                    compiler,
                    expr_to_assignment_inst(
                        compiler, list_content_type, NULL_IDENT, args->values[0]
                    )
                );
                check_assignment_type_info(compiler, assignment, INT_TYPE);
                assignment->right = c_function_call(
                    compiler,
                    NPLIB_LIST_INDEX,
                    ident(list_assignment.left),
                    REFERENCE_PREVIOUS_ASSIGNMENT(compiler),
                    NULL
                );
                return;
            }
            else if (strcmp(fn_name, "insert") == 0) {
                expect_arg_count(compiler, "list.insert", args, 2);
                Instruction index_inst = expr_to_assignment_inst(
                    compiler, (TypeInfo){.type = NPTYPE_INT}, NULL_IDENT, args->values[0]
                );
                Instruction item_inst = expr_to_assignment_inst(
                    compiler, list_content_type, NULL_IDENT, args->values[1]
                );
                add_instruction(compiler, index_inst);
                add_instruction(compiler, item_inst);
                check_assignment_type_info(compiler, assignment, NONE_TYPE);
                assignment->right = c_function_call(
                    compiler,
                    NPLIB_LIST_INSERT,
                    ident(list_assignment.left),
                    ident(index_inst.assignment.left),
                    REFERENCE_ASSIGNMENT(compiler, item_inst.assignment),
                    NULL
                );
                return;
            }
            break;
        case 'p':
            if (strcmp(fn_name, "pop") == 0) {
                const char* index_ident;
                if (args->values_count == 0)
                    index_ident = "-1";
                else if (args->values_count == 1) {
                    add_instruction(
                        compiler,
                        expr_to_assignment_inst(
                            compiler,
                            (TypeInfo){.type = NPTYPE_INT},
                            NULL_IDENT,
                            args->values[0]
                        )
                    );
                    index_ident = ident(compiler->previous_instruction.assignment.left);
                }
                else {
                    type_errorf(
                        compiler->file_index,
                        compiler->current_operation_location,
                        "list.pop expecting 0-1 arguments but got %zu",
                        args->values_count
                    );
                }
                const char* out_ident = generate_ident(compiler);
                add_instruction(
                    compiler,
                    (Instruction){
                        .kind = INST_DECLARE_VARIABLE,
                        .declare_variable.ident.cstr = out_ident,
                        .declare_variable.type_info = list_content_type}
                );
                add_instruction(
                    compiler,
                    (Instruction){
                        .kind = INST_EXPRESSION,
                        .expression = c_function_call(
                            compiler,
                            NPLIB_LIST_POP,
                            ident(list_assignment.left),
                            index_ident,
                            REFERENCE(compiler, out_ident),
                            NULL
                        )}
                );

                check_assignment_type_info(compiler, assignment, list_content_type);
                assignment->right = out_ident;
                return;
            }
            break;
        case 'r':
            if (strcmp(fn_name, "remove") == 0) {
                expect_arg_count(compiler, "list.remove", args, 1);
                add_instruction(
                    compiler,
                    expr_to_assignment_inst(
                        compiler, list_content_type, NULL_IDENT, args->values[0]
                    )
                );
                check_assignment_type_info(compiler, assignment, NONE_TYPE);
                assignment->right = c_function_call(
                    compiler,
                    NPLIB_LIST_REMOVE,
                    ident(list_assignment.left),
                    REFERENCE_PREVIOUS_ASSIGNMENT(compiler),
                    NULL
                );
                return;
            }
            else if (strcmp(fn_name, "reverse") == 0) {
                expect_arg_count(compiler, "list.reverse", args, 0);
                check_assignment_type_info(compiler, assignment, NONE_TYPE);
                assignment->right = c_function_call(
                    compiler, NPLIB_LIST_REVERSE, ident(list_assignment.left), NULL
                );
                return;
            }
            break;
        case 's':
            if (strcmp(fn_name, "sort") == 0) {
                // TODO: accept key function argument
                // validate args
                static const SourceString REVERSE = {.data = "reverse", .length = 7};
                bool bad_args = false;
                if (args->values_count != 0) {
                    if (args->n_positional > 0)
                        bad_args = true;
                    else if (args->values_count > 1)
                        bad_args = true;
                    else if (!(SOURCESTRING_EQ(args->kwds[0], REVERSE)))
                        bad_args = true;
                }
                if (bad_args) {
                    fprintf(
                        stderr,
                        "ERROR: list.sort expecting either 0 args or boolean keyword "
                        "arg "
                        "`reverse`\n"
                    );
                    exit(1);
                }
                // resolve `reverse` argument
                const char* reversed_arg = NULL;
                if (args->values_count > 0) {
                    Instruction reversed_inst = expr_to_assignment_inst(
                        compiler, UNTYPED, NULL_IDENT, args->values[0]
                    );
                    add_instruction(compiler, reversed_inst);
                    reversed_arg =
                        assignment_to_truthy(compiler, reversed_inst.assignment);
                }
                else {
                    reversed_arg = "false";
                }
                // compile
                check_assignment_type_info(compiler, assignment, NONE_TYPE);
                assignment->right = c_function_call(
                    compiler,
                    NPLIB_LIST_SORT,
                    ident(list_assignment.left),
                    reversed_arg,
                    NULL
                );
                return;
            }
            break;
        default:
            name_errorf(
                compiler->file_index,
                compiler->current_operation_location,
                "unrecognized list builtin: %s",
                fn_name
            );
    }
}

static void
compile_dict_builtin(
    C_Compiler* compiler,
    AssignmentInst* assignment,
    AssignmentInst dict_assignment,
    const char* fn_name,
    Arguments* args
)
{
    assert(fn_name && "fn_name cannot be NULL");

    TypeInfo key_type = dict_assignment.type_info.inner->types[0];
    TypeInfo val_type = dict_assignment.type_info.inner->types[1];

    switch (fn_name[0]) {
        case 'c':
            if (strcmp(fn_name, "clear") == 0) {
                expect_arg_count(compiler, "dict.clear", args, 0);
                check_assignment_type_info(compiler, assignment, NONE_TYPE);
                assignment->right = c_function_call(
                    compiler, NPLIB_DICT_CLEAR, ident(dict_assignment.left), NULL
                );
                return;
            }
            else if (strcmp(fn_name, "copy") == 0) {
                expect_arg_count(compiler, "dict.copy", args, 0);
                check_assignment_type_info(
                    compiler, assignment, dict_assignment.type_info
                );
                assignment->right = c_function_call(
                    compiler, NPLIB_DICT_COPY, ident(dict_assignment.left), NULL
                );
                return;
            }
            break;
        case 'g':
            if (strcmp(fn_name, "get") == 0) {
                UNIMPLEMENTED("dict.get is not implemented");
                return;
            }
            break;
        case 'i':
            if (strcmp(fn_name, "items") == 0) {
                expect_arg_count(compiler, "dict.items", args, 0);

                // Iter[DictItems[KeyType, ValueType]]
                TypeInfo dict_items_type = {
                    .type = NPTYPE_DICT_ITEMS, .inner = dict_assignment.type_info.inner};
                TypeInfo return_type = {
                    .type = NPTYPE_ITER,
                    .inner = arena_alloc(compiler->arena, sizeof(TypeInfoInner))};
                return_type.inner->types = arena_alloc(compiler->arena, sizeof(TypeInfo));
                return_type.inner->types[0] = dict_items_type;
                return_type.inner->count = 1;

                check_assignment_type_info(compiler, assignment, return_type);
                assignment->right = c_function_call(
                    compiler, NPLIB_DICT_ITEMS, ident(dict_assignment.left), NULL
                );
                return;
            }
            break;
        case 'k':
            if (strcmp(fn_name, "keys") == 0) {
                expect_arg_count(compiler, "dict.keys", args, 0);

                // Iter[key type]
                TypeInfo return_type = {
                    .type = NPTYPE_ITER,
                    .inner = arena_alloc(compiler->arena, sizeof(TypeInfoInner))};
                return_type.inner->types = arena_alloc(compiler->arena, sizeof(TypeInfo));
                return_type.inner->types[0] = key_type;
                return_type.inner->count = 1;

                check_assignment_type_info(compiler, assignment, return_type);
                assignment->right = c_function_call(
                    compiler, NPLIB_DICT_KEYS, ident(dict_assignment.left), NULL
                );
                return;
            }
            break;
        case 'p':
            if (strcmp(fn_name, "pop") == 0) {
                // TODO: implement default value
                expect_arg_count(compiler, "dict.pop", args, 1);

                Instruction key_inst = expr_to_assignment_inst(
                    compiler,
                    dict_assignment.type_info.inner->types[0],
                    NULL_IDENT,
                    args->values[0]
                );
                add_instruction(compiler, key_inst);
                const char* out_ident = generate_ident(compiler);
                add_instruction(
                    compiler,
                    (Instruction){
                        .kind = INST_DECLARE_VARIABLE,
                        .declare_variable.ident.cstr = out_ident,
                        .declare_variable.type_info = val_type}
                );
                add_instruction(
                    compiler,
                    (Instruction){
                        .kind = INST_EXPRESSION,
                        .expression = c_function_call(
                            compiler,
                            NPLIB_DICT_POP,
                            ident(dict_assignment.left),
                            REFERENCE_ASSIGNMENT(compiler, key_inst.assignment),
                            REFERENCE(compiler, out_ident),
                            NULL
                        ),
                    }
                );

                check_assignment_type_info(compiler, assignment, val_type);
                assignment->right = out_ident;
                return;
            }
            break;
            if (strcmp(fn_name, "popitem") == 0) {
                UNIMPLEMENTED(
                    "dict.popitem will not be implemented until tuples are "
                    "implemented"
                );
                return;
            }
            break;
        case 'u':
            if (strcmp(fn_name, "update") == 0) {
                // TODO: accept args other than another dict
                expect_arg_count(compiler, "dict.update", args, 1);

                add_instruction(
                    compiler,
                    expr_to_assignment_inst(
                        compiler, dict_assignment.type_info, NULL_IDENT, args->values[0]
                    )
                );
                check_assignment_type_info(compiler, assignment, NONE_TYPE);
                assignment->right = c_function_call(
                    compiler,
                    NPLIB_DICT_UPDATE,
                    ident(dict_assignment.left),
                    ident(compiler->previous_instruction.assignment.left),
                    NULL
                );
                return;
            }
            break;
        case 'v':
            if (strcmp(fn_name, "values") == 0) {
                expect_arg_count(compiler, "dict.values", args, 0);

                // ITER of VALUE_TYPE
                TypeInfo return_type = {
                    .type = NPTYPE_ITER,
                    .inner = arena_alloc(compiler->arena, sizeof(TypeInfoInner))};
                return_type.inner->types = arena_alloc(compiler->arena, sizeof(TypeInfo));
                return_type.inner->types[0] = val_type;
                return_type.inner->count = 1;

                check_assignment_type_info(compiler, assignment, return_type);
                assignment->right = c_function_call(
                    compiler, NPLIB_DICT_VALUES, ident(dict_assignment.left), NULL
                );
                return;
            }
            break;
        default:
            name_errorf(
                compiler->file_index,
                compiler->current_operation_location,
                "unrecognized dict builtin: %s",
                fn_name
            );
    }
}

// TODO: parser will need to enforce that builtins dont get defined by the user
static void
render_builtin(
    C_Compiler* compiler,
    AssignmentInst* assignment,
    const char* fn_identifier,
    Arguments* args
)
{
    if (strcmp(fn_identifier, "print") == 0) {
        render_builtin_print(compiler, assignment, args);
        return;
    }
    else {
        name_errorf(
            compiler->file_index,
            compiler->current_operation_location,
            "function not found: %s",
            fn_identifier
        );
    }
}

static const char*
assignment_to_truthy(C_Compiler* compiler, AssignmentInst assignment)
{
    switch (assignment.type_info.type) {
        case NPTYPE_EXCEPTION:
            return ident(assignment.left);
        case NPTYPE_FUNCTION:
            return sb_build_cstr(&compiler->sb, ident(assignment.left), ".addr", NULL);
        case NPTYPE_INT:
            return ident(assignment.left);
        case NPTYPE_FLOAT:
            return ident(assignment.left);
        case NPTYPE_BOOL:
            return ident(assignment.left);
        case NPTYPE_STRING:
            return sb_build_cstr(&compiler->sb, ident(assignment.left), ".length", NULL);
        case NPTYPE_LIST:
            return sb_build_cstr(&compiler->sb, ident(assignment.left), "->count", NULL);
        case NPTYPE_DICT:
            return sb_build_cstr(&compiler->sb, ident(assignment.left), "->count", NULL);
        case NPTYPE_NONE:
            return "0";
        case NPTYPE_ITER:
            UNIMPLEMENTED("truthy conversion unimplemented for NPTYPE_ITER");
        case NPTYPE_OBJECT: {
            ClassStatement* clsdef = assignment.type_info.cls;
            FunctionStatement* fndef = clsdef->object_model_methods[OBJECT_MODEL_BOOL];
            if (!fndef) return ident(assignment.left);
            return c_function_call(
                compiler,
                // cast NpFunction object into a function pointer
                sb_c_cast(
                    &compiler->sb,
                    sb_build_cstr(&compiler->sb, fndef->ns_ident.data, ".addr", NULL),
                    (TypeInfo){.type = NPTYPE_FUNCTION, .sig = &fndef->sig}
                ),
                // pass in context with `self` bound
                COMPILER_STRING(
                    compiler, "(NpContext){.self = ", ident(assignment.left), "}"
                ),
                NULL
            );
        }
        case NPTYPE_SLICE:
            UNREACHABLE();
        case NPTYPE_DICT_ITEMS:
            UNREACHABLE();
        case NPTYPE_UNTYPED:
            UNREACHABLE();
        case NPTYPE_TUPLE:
            UNREACHABLE();
        case NPTYPE_CONTEXT:
            UNREACHABLE();
        case NPTYPE_COUNT:
            UNREACHABLE();
    }
    UNREACHABLE();
}

static void
require_math_lib(C_Compiler* compiler)
{
    if (compiler->reqs.libs[LIB_MATH]) return;
    compiler->reqs.libs[LIB_MATH] = true;
}

static void
render_object_operation(
    C_Compiler* compiler,
    AssignmentInst* assignment,
    Operator op_type,
    const char** operand_reprs,
    TypeInfo* types
)
{
    bool is_rop;
    bool is_unary;

    FunctionStatement* fndef =
        find_object_op_function(types[0], types[1], op_type, &is_rop, &is_unary);

    if (!fndef)
        type_errorf(
            compiler->file_index,
            compiler->current_operation_location,
            "unsupported operand types for `%s`: `%s` and `%s`",
            op_to_cstr(op_type),
            errfmt_type_info(types[0]),
            errfmt_type_info(types[1])
        );

    // create a context containing `self`:
    const char* ctx_ident = generate_ident(compiler);
    add_instruction(
        compiler,
        (Instruction){
            .kind = INST_DECLARE_VARIABLE,
            .declare_variable.ident.cstr = ctx_ident,
            .declare_variable.type_info.type = NPTYPE_CONTEXT,
        }
    );
    add_instruction(
        compiler,
        (Instruction){
            .kind = INST_ASSIGNMENT,
            .assignment.type_info.type = NPTYPE_CONTEXT,
            .assignment.left.cstr = ctx_ident,
            .assignment.right =
                sb_build_cstr(&compiler->sb, fndef->ns_ident.data, ".ctx", NULL),
        }
    );
    add_instruction(
        compiler,
        (Instruction){
            .kind = INST_ASSIGNMENT,
            .assignment.type_info = (is_rop) ? types[1] : types[0],
            .assignment.left.cstr =
                sb_build_cstr(&compiler->sb, ctx_ident, ".self", NULL),
            .assignment.right = (is_rop) ? operand_reprs[1] : operand_reprs[0],
        }
    );

    // cast NpFunction struct to function pointer
    const char* fn_pointer = sb_c_cast(
        &compiler->sb,
        COMPILER_STRING(compiler, fndef->ns_ident.data, ".addr"),
        (TypeInfo){.type = NPTYPE_FUNCTION, .sig = &fndef->sig}
    );

    check_assignment_type_info(compiler, assignment, fndef->sig.return_type);

    if (is_unary)
        assignment->right = c_function_call(compiler, fn_pointer, ctx_ident, NULL);
    else if (is_rop)
        assignment->right =
            c_function_call(compiler, fn_pointer, ctx_ident, operand_reprs[0], NULL);
    else
        assignment->right =
            c_function_call(compiler, fn_pointer, ctx_ident, operand_reprs[1], NULL);
}

static void
render_operation(
    C_Compiler* compiler,
    AssignmentInst* assignment,
    Operator op_type,
    const char** operand_reprs,
    TypeInfo* types
)
{
    if (types[0].type == NPTYPE_OBJECT || types[1].type == NPTYPE_OBJECT) {
        render_object_operation(compiler, assignment, op_type, operand_reprs, types);
        return;
    }

    TypeInfo type_info = resolve_operation_type(types[0], types[1], op_type);
    if (type_info.type == NPTYPE_UNTYPED) {
        type_errorf(
            compiler->file_index,
            compiler->current_operation_location,
            "unsupported operand types for `%s`: `%s` and `%s`",
            op_to_cstr(op_type),
            errfmt_type_info(types[0]),
            errfmt_type_info(types[1])
        );
    }
    check_assignment_type_info(compiler, assignment, type_info);

    switch (op_type) {
        case OPERATOR_LOGICAL_NOT: {
            AssignmentInst operand = {
                .left.cstr = operand_reprs[1], .type_info = types[1]};
            assignment->right = sb_build_cstr(
                &compiler->sb, "(!", assignment_to_truthy(compiler, operand), ");\n", NULL
            );
            return;
        }
        case OPERATOR_IS: {
            bool cmp_address = types[0].type == NPTYPE_STRING;
            assignment->right = sb_build_cstr(
                &compiler->sb,
                (cmp_address) ? REFERENCE(compiler, operand_reprs[0]) : operand_reprs[0],
                " == ",
                (cmp_address) ? REFERENCE(compiler, operand_reprs[1]) : operand_reprs[1],
                NULL
            );
            return;
        }
        case OPERATOR_PLUS:
            if (types[0].type == NPTYPE_STRING) {
                assignment->right =
                    c_function_calln(compiler, NPLIB_STR_ADD, operand_reprs, 2);
                return;
            }
            else if (types[0].type == NPTYPE_LIST) {
                assignment->right =
                    c_function_calln(compiler, NPLIB_LIST_ADD, operand_reprs, 2);
                return;
            }
            break;
        case OPERATOR_DIV:
            assignment->right = sb_build_cstr(
                &compiler->sb,
                (types[0].type == NPTYPE_INT) ? "(NpFloat)" : "",
                operand_reprs[0],
                "/",
                (types[1].type == NPTYPE_INT) ? "(NpFloat)" : "",
                operand_reprs[1],
                NULL
            );
            return;
        case OPERATOR_FLOORDIV:
            assignment->right = sb_build_cstr(
                &compiler->sb,
                "(NpInt)(",
                (types[0].type == NPTYPE_INT) ? "(NpFloat)" : "",
                operand_reprs[0],
                "/",
                (types[1].type == NPTYPE_INT) ? "(NpFloat)" : "",
                operand_reprs[1],
                ")",
                NULL
            );
            return;
        case OPERATOR_MOD:
            if (assignment->type_info.type == NPTYPE_FLOAT) {
                require_math_lib(compiler);
                assignment->right =
                    c_function_calln(compiler, NPLIB_FMOD, operand_reprs, 2);
            }
            else {
                assignment->right = sb_build_cstr(
                    &compiler->sb, operand_reprs[0], "%", operand_reprs[1], NULL
                );
            }
            return;

        case OPERATOR_POW:
            require_math_lib(compiler);
            assignment->right = sb_build_cstr(
                &compiler->sb,
                (assignment->type_info.type == NPTYPE_INT) ? "(NpInt)" : "",
                c_function_calln(compiler, NPLIB_POW, operand_reprs, 2),
                NULL
            );
            return;
        case OPERATOR_EQUAL:
            if (types[0].type == NPTYPE_STRING) {
                assignment->right =
                    c_function_calln(compiler, NPLIB_STR_EQ, operand_reprs, 2);
                return;
            }
            // break out to default
            break;
        case OPERATOR_GREATER:
            if (types[0].type == NPTYPE_STRING) {
                assignment->right =
                    c_function_calln(compiler, NPLIB_STR_GT, operand_reprs, 2);
                return;
            }
            // break out to default
            break;
        case OPERATOR_GREATER_EQUAL:
            if (types[0].type == NPTYPE_STRING) {
                assignment->right =
                    c_function_calln(compiler, NPLIB_STR_GTE, operand_reprs, 2);
                return;
            }
            // break out to default
            break;
        case OPERATOR_LESS:
            if (types[0].type == NPTYPE_STRING) {
                assignment->right =
                    c_function_calln(compiler, NPLIB_STR_LT, operand_reprs, 2);
                return;
            }
            // break out to default
            break;
        case OPERATOR_LESS_EQUAL:
            if (types[0].type == NPTYPE_STRING && types[0].type == NPTYPE_STRING) {
                assignment->right =
                    c_function_calln(compiler, NPLIB_STR_LTE, operand_reprs, 2);
                return;
            }
            // break out to default
            break;
        case OPERATOR_GET_ITEM:
            // TODO: return void* and cast+deref after call for containers
            // also offer library macro for doing this / maybe even use it

            if (types[0].type == NPTYPE_LIST) {
                if (types[1].type == NPTYPE_SLICE)
                    UNIMPLEMENTED("list slicing unimplemented");
                const char* out_id = generate_ident(compiler);

                add_instruction(
                    compiler,
                    (Instruction){
                        .kind = INST_DECLARE_VARIABLE,
                        .declare_variable.ident.cstr = out_id,
                        .declare_variable.type_info = type_info}
                );
                add_instruction(
                    compiler,
                    (Instruction){
                        .kind = INST_EXPRESSION,
                        .expression = c_function_call(
                            compiler,
                            NPLIB_LIST_GET_ITEM,
                            operand_reprs[0],
                            operand_reprs[1],
                            REFERENCE(compiler, out_id),
                            NULL
                        ),
                    }
                );
                assignment->right = out_id;
                return;
            }

            else if (types[0].type == NPTYPE_DICT) {
                const char* out_id = generate_ident(compiler);
                add_instruction(
                    compiler,
                    (Instruction){
                        .kind = INST_DECLARE_VARIABLE,
                        .declare_variable.ident.cstr = out_id,
                        .declare_variable.type_info = type_info}
                );
                add_instruction(
                    compiler,
                    (Instruction){
                        .kind = INST_EXPRESSION,
                        .expression = c_function_call(
                            compiler,
                            NPLIB_DICT_GET_ITEM,
                            operand_reprs[0],
                            REFERENCE(compiler, operand_reprs[1]),
                            REFERENCE(compiler, out_id),
                            NULL
                        ),
                    }
                );
                assignment->right = out_id;
                return;
            }
            else {
                UNIMPLEMENTED("getitem unimplemented for this type");
            }
            // break out to default
            break;
        default:
            // break out to default
            break;
    }
    assignment->right = COMPILER_STRING(
        compiler, operand_reprs[0], op_to_cstr(op_type), operand_reprs[1]
    );
    return;
}

static void
render_empty_enclosure(C_Compiler* compiler, AssignmentInst* assignment, Operand operand)
{
    assert(
        assignment->type_info.type != NPTYPE_UNTYPED && "assignment assumed to be typed"
    );
    switch (operand.enclosure->type) {
        case ENCLOSURE_LIST: {
            TypeInfo list_content_type = assignment->type_info.inner->types[0];
            const char* rev_sort = sort_cmp_for_type_info(list_content_type, true);
            const char* norm_sort = sort_cmp_for_type_info(list_content_type, false);
            const char* cmp = voidptr_cmp_for_type_info(list_content_type);

            assignment->right = c_function_call(
                compiler,
                NPLIB_LIST_INIT,
                type_info_to_c_syntax(&compiler->sb, list_content_type),
                COMPILER_STRING(compiler, "(NpSortFunction)", norm_sort),
                COMPILER_STRING(compiler, "(NpSortFunction)", rev_sort),
                COMPILER_STRING(compiler, "(NpCompareFunction)", cmp),
                NULL
            );
            break;
        }
        case ENCLOSURE_DICT: {
            TypeInfo key_type = assignment->type_info.inner->types[0];
            assignment->right = c_function_call(
                compiler,
                NPLIB_DICT_INIT,
                type_info_to_c_syntax(&compiler->sb, key_type),
                type_info_to_c_syntax(
                    &compiler->sb, assignment->type_info.inner->types[1]
                ),
                voidptr_cmp_for_type_info(key_type),
                NULL
            );
            break;
        }
        case ENCLOSURE_TUPLE:
            UNIMPLEMENTED("empty tuple rendering unimplemented");
        default:
            UNREACHABLE();
    }
}

static void
render_list_literal(C_Compiler* compiler, AssignmentInst* assignment, Operand operand)
{
    // we know the list is not empty

    const bool untyped = (assignment->type_info.type == NPTYPE_UNTYPED);

    Instruction list_element_assignment = expr_to_assignment_inst(
        compiler,
        (untyped) ? UNTYPED : assignment->type_info.inner->types[0],
        NULL_IDENT,
        operand.enclosure->expressions[0]
    );
    add_instruction(compiler, list_element_assignment);

    if (untyped) {
        TypeInfoInner* inner = arena_alloc(compiler->arena, sizeof(TypeInfoInner));
        inner->types = arena_alloc(compiler->arena, sizeof(TypeInfo));
        inner->types[0] = compiler->previous_instruction.assignment.type_info;
        inner->count = 1;
        set_assignment_type_info(
            compiler, assignment, (TypeInfo){.type = NPTYPE_LIST, .inner = inner}
        );
    }

    // TODO: for now we're just going to init an empty list and append everything
    // to it. eventually we should allocate enough room to begin with because we
    // already know the length of the list
    const char* tmpid = generate_ident(compiler);
    add_instruction(
        compiler,
        (Instruction){
            .kind = INST_DECLARE_VARIABLE,
            .declare_variable.type_info = assignment->type_info,
            .declare_variable.ident.cstr = tmpid,
        }
    );
    Instruction init_list = {
        .kind = INST_ASSIGNMENT,
        .assignment.left.cstr = tmpid,
        .assignment.type_info = assignment->type_info,
    };
    render_empty_enclosure(compiler, &init_list.assignment, operand);
    add_instruction(compiler, init_list);

    size_t i = 1;
    for (;;) {
        add_instruction(
            compiler,
            (Instruction){
                .kind = INST_EXPRESSION,
                .expression = c_function_call(
                    compiler,
                    NPLIB_LIST_APPEND,
                    tmpid,
                    REFERENCE_INSTRUCTION(compiler, list_element_assignment),
                    NULL
                )}
        );
        if (i == operand.enclosure->expressions_count) break;
        list_element_assignment = expr_to_assignment_inst(
            compiler,
            assignment->type_info.inner->types[0],
            list_element_assignment.assignment.left,
            operand.enclosure->expressions[i++]
        );
        add_instruction(compiler, list_element_assignment);
    }

    assignment->right = tmpid;
}

static void
render_dict_literal(C_Compiler* compiler, AssignmentInst* assignment, Operand operand)
{
    TypeInfo key_info = UNTYPED;
    TypeInfo val_info = UNTYPED;
    if (assignment->type_info.type != NPTYPE_UNTYPED) {
        // if we already know the type enforce it
        key_info = assignment->type_info.inner->types[0];
        val_info = assignment->type_info.inner->types[1];
    }
    Instruction key_inst = expr_to_assignment_inst(
        compiler, key_info, NULL_IDENT, operand.enclosure->expressions[0]
    );
    Instruction val_inst = expr_to_assignment_inst(
        compiler, val_info, NULL_IDENT, operand.enclosure->expressions[1]
    );
    add_instruction(compiler, key_inst);
    add_instruction(compiler, val_inst);
    const char* key_ident = ident(key_inst.assignment.left);
    const char* val_ident = ident(val_inst.assignment.left);

    // if we don't know the type already then this is the defining statement
    if (assignment->type_info.type == NPTYPE_UNTYPED) {
        TypeInfoInner* inner = arena_alloc(compiler->arena, sizeof(TypeInfoInner));
        inner->types = arena_alloc(compiler->arena, sizeof(TypeInfo) * 2);
        inner->types[0] = key_inst.assignment.type_info;
        inner->types[1] = val_inst.assignment.type_info;
        inner->count = 2;
        set_assignment_type_info(
            compiler, assignment, (TypeInfo){.type = NPTYPE_DICT, .inner = inner}
        );
    }

    // TODO: better initialization
    const char* tmpid = generate_ident(compiler);
    add_instruction(
        compiler,
        (Instruction){
            .kind = INST_DECLARE_VARIABLE,
            .declare_variable.type_info = assignment->type_info,
            .declare_variable.ident.cstr = tmpid,
        }
    );
    Instruction init_dict = {
        .kind = INST_ASSIGNMENT,
        .assignment.left.cstr = tmpid,
        .assignment.type_info = assignment->type_info};
    render_empty_enclosure(compiler, &init_dict.assignment, operand);
    add_instruction(compiler, init_dict);

    size_t expression_index = 2;
    for (;;) {
        add_instruction(
            compiler,
            (Instruction){
                .kind = INST_EXPRESSION,
                .expression = c_function_call(
                    compiler,
                    NPLIB_DICT_SET_ITEM,
                    tmpid,
                    REFERENCE(compiler, key_ident),
                    REFERENCE(compiler, val_ident),
                    NULL
                )}
        );
        if (expression_index == operand.enclosure->expressions_count) break;
        add_instruction(
            compiler,
            expr_to_assignment_inst(
                compiler,
                key_info,
                (Ident){.cstr = key_ident},
                operand.enclosure->expressions[expression_index++]
            )
        );
        add_instruction(
            compiler,
            expr_to_assignment_inst(
                compiler,
                val_info,
                (Ident){.cstr = val_ident},
                operand.enclosure->expressions[expression_index++]
            )
        );
    }
    assignment->right = tmpid;
}

static void
render_enclosure_literal(
    C_Compiler* compiler, AssignmentInst* assignment, Operand operand
)
{
    if (operand.enclosure->expressions_count == 0) {
        if (assignment->type_info.type == NPTYPE_UNTYPED) {
            type_error(
                compiler->file_index,
                compiler->current_stmt_location,
                "empty containers must have their type annotated when initialized"
            );
        }
        render_empty_enclosure(compiler, assignment, operand);
        return;
    }

    switch (operand.enclosure->type) {
        case ENCLOSURE_LIST:
            render_list_literal(compiler, assignment, operand);
            break;
        case ENCLOSURE_DICT:
            render_dict_literal(compiler, assignment, operand);
            break;
        case ENCLOSURE_TUPLE:
            UNIMPLEMENTED("rendering of tuple enclosure literal unimplemented");
        default:
            UNREACHABLE();
    }
}

static void
render_token_operand(C_Compiler* compiler, AssignmentInst* assignment, Operand operand)
{
    TypeInfo info = resolve_operand_type(&compiler->tc, operand);
    if (info.type == NPTYPE_UNTYPED) {
        // TODO: function to determine if this is a builtin function which cannot be
        // referenced and give a more informative error message.
        type_errorf(
            compiler->file_index,
            *operand.token.loc,
            "unable to resolve the type for token `%s`",
            operand.token.value.data
        );
    }
    check_assignment_type_info(compiler, assignment, info);
    assignment->right = token_operand_repr(compiler, operand);
}

static void
render_operand(C_Compiler* compiler, AssignmentInst* assignment, Operand operand)
{
    switch (operand.kind) {
        case OPERAND_ENCLOSURE_LITERAL:
            render_enclosure_literal(compiler, assignment, operand);
            break;
        case OPERAND_COMPREHENSION:
            UNIMPLEMENTED("comprehension rendering unimplemented");
            break;
        case OPERAND_SLICE:
            UNIMPLEMENTED("slice rendering unimplemented");
            break;
        case OPERAND_EXPRESSION: {
            Instruction intermediate;
            add_instruction(
                compiler,
                intermediate = expr_to_assignment_inst(
                    compiler, assignment->type_info, NULL_IDENT, operand.expr
                )
            );
            assignment->right = ident(intermediate.assignment.left);
            check_assignment_type_info(
                compiler, assignment, intermediate.assignment.type_info
            );
            break;
        }
        case OPERAND_TOKEN:
            render_token_operand(compiler, assignment, operand);
            break;
        default:
            UNREACHABLE();
            break;
    }
}

static void
render_type_hinted_signature_args_to_variables(
    C_Compiler* compiler,
    Arguments* args,
    Signature sig,
    const char* callable_name,
    const char** variable_identifiers
)
{
    if (args->values_count != sig.params_count) {
        type_errorf(
            compiler->file_index,
            compiler->current_operation_location,
            "callable `%s` takes %zu arguments but %zu were given",
            callable_name,
            sig.params_count,
            args->values_count
        );
    }
    if (args->values_count != args->n_positional) {
        type_errorf(
            compiler->file_index,
            compiler->current_operation_location,
            "callable `%s` derives it's type from a type hint and does not take "
            "keyword "
            "arguments keyword arguments",
            callable_name
        );
    }
    for (size_t arg_i = 0; arg_i < args->values_count; arg_i++) {
        add_instruction(
            compiler,
            expr_to_assignment_inst(
                compiler, sig.types[arg_i], NULL_IDENT, args->values[arg_i]
            )
        );
        variable_identifiers[arg_i] = inst_ident(compiler->previous_instruction);
    }
}

static void
render_callable_args_to_variables(
    C_Compiler* compiler,
    Arguments* args,
    Signature sig,
    const char* callable_name,
    const char** variable_identifiers
)
{
    if (!sig.params) {
        // Signature coming from a type hint, only accepts positional args
        render_type_hinted_signature_args_to_variables(
            compiler, args, sig, callable_name, variable_identifiers
        );
        return;
    }
    if (args->values_count > sig.params_count) {
        type_errorf(
            compiler->file_index,
            compiler->current_operation_location,
            "callable `%s` takes %zu arguments but %zu were given",
            callable_name,
            sig.params_count,
            args->values_count
        );
    }

    bool params_fulfilled[sig.params_count];
    memset(params_fulfilled, true, sizeof(bool) * args->n_positional);
    memset(
        params_fulfilled + args->n_positional,
        false,
        sizeof(bool) * (sig.params_count - args->n_positional)
    );

    // positional args
    for (size_t arg_i = 0; arg_i < args->n_positional; arg_i++) {
        add_instruction(
            compiler,
            expr_to_assignment_inst(
                compiler, sig.types[arg_i], NULL_IDENT, args->values[arg_i]
            )
        );
        variable_identifiers[arg_i] = inst_ident(compiler->previous_instruction);
    }

    // parse kwargs
    for (size_t arg_i = args->n_positional; arg_i < args->values_count; arg_i++) {
        SourceString kwd = args->kwds[arg_i - args->n_positional];
        int param_i = index_of_kwarg(sig, kwd);
        if (param_i < 0) {
            type_errorf(
                compiler->file_index,
                compiler->current_operation_location,
                "callable `%s` was given an unexpected keyword argument `%s`",
                callable_name,
                kwd.data
            );
        }
        params_fulfilled[param_i] = true;
        add_instruction(
            compiler,
            expr_to_assignment_inst(
                compiler, sig.types[param_i], NULL_IDENT, args->values[arg_i]
            )
        );
        variable_identifiers[param_i] = inst_ident(compiler->previous_instruction);
    }

    // check that all required params are fulfilled
    size_t required_count = sig.params_count - sig.defaults_count;
    for (size_t i = 0; i < required_count; i++) {
        if (!params_fulfilled[i]) {
            type_errorf(
                compiler->file_index,
                compiler->current_operation_location,
                "callable `%s` missing required param `%s`",
                callable_name,
                sig.params[i].data
            );
        }
    }

    // fill in any unprovided values with their default expressions
    for (size_t i = required_count; i < sig.params_count; i++) {
        if (params_fulfilled[i])
            continue;
        else {
            add_instruction(
                compiler,
                expr_to_assignment_inst(
                    compiler, sig.types[i], NULL_IDENT, sig.defaults[i - required_count]
                )
            );
            variable_identifiers[i] = inst_ident(compiler->previous_instruction);
        }
    }
}

static void
render_object_method_call(
    C_Compiler* compiler,
    AssignmentInst* assignment,
    AssignmentInst self_assignment,
    FunctionStatement* fndef,
    Arguments* args
)
{
    // intermediate variables to store args into
    size_t argc = 1 + fndef->sig.params_count;
    const char* arg_variable_idents[argc];
    memset(arg_variable_idents, 0, sizeof(arg_variable_idents));
    render_callable_args_to_variables(
        compiler, args, fndef->sig, fndef->name.data, arg_variable_idents + 1
    );

    // create a context containing `self`:
    const char* ctx_ident = generate_ident(compiler);
    arg_variable_idents[0] = ctx_ident;
    add_instruction(
        compiler,
        (Instruction){
            .kind = INST_DECLARE_VARIABLE,
            .declare_variable.type_info.type = NPTYPE_CONTEXT,
            .declare_variable.ident.cstr = ctx_ident,
        }
    );
    add_instruction(
        compiler,
        (Instruction){
            .kind = INST_ASSIGNMENT,
            .assignment.type_info.type = NPTYPE_CONTEXT,
            .assignment.left.cstr = ctx_ident,
            .assignment.right = COMPILER_STRING(compiler, fndef->ns_ident.data, ".ctx")}
    );
    add_instruction(
        compiler,
        (Instruction){
            .kind = INST_ASSIGNMENT,
            .assignment.type_info = self_assignment.type_info,
            .assignment.left.cstr = COMPILER_STRING(compiler, ctx_ident, ".self"),
            .assignment.right = ident(self_assignment.left)}
    );

    // render call
    const char* fn_ptr = sb_c_cast(
        &compiler->sb,
        sb_build_cstr(&compiler->sb, fndef->ns_ident.data, ".addr", NULL),
        (TypeInfo){.type = NPTYPE_FUNCTION, .sig = &fndef->sig}
    );
    check_assignment_type_info(compiler, assignment, fndef->sig.return_type);
    assignment->right = c_function_calln(compiler, fn_ptr, arg_variable_idents, argc);
}

static void
render_object_creation(
    C_Compiler* compiler,
    AssignmentInst* assignment,
    ClassStatement* clsdef,
    Arguments* args
)
{
    // `type` var = np_alloc(bytes);
    check_assignment_type_info(compiler, assignment, clsdef->sig.return_type);

    const char* tmp_ident = generate_ident(compiler);
    assignment->right = tmp_ident;
    add_instruction(
        compiler,
        (Instruction){
            .kind = INST_DECLARE_VARIABLE,
            .declare_variable.ident.cstr = tmp_ident,
            .declare_variable.type_info = clsdef->sig.return_type}
    );
    Instruction tmp_self = compiler->previous_instruction;
    add_instruction(
        compiler,
        (Instruction){
            .kind = INST_ASSIGNMENT,
            .assignment.left.cstr = tmp_ident,
            .assignment.right = c_function_call(
                compiler,
                NPLIB_ALLOC,
                COMPILER_STRING(compiler, "sizeof(", clsdef->ns_ident.data, ")"),
                NULL
            ),
            .assignment.type_info = clsdef->sig.return_type}
    );

    // use user defined __init__ method
    FunctionStatement* init_def = clsdef->object_model_methods[OBJECT_MODEL_INIT];
    if (init_def) {
        for (size_t i = 0; i < clsdef->sig.defaults_count; i++) {
            // set defaults
            add_instruction(
                compiler,
                expr_to_assignment_inst(
                    compiler,
                    clsdef->sig.types[clsdef->sig.params_count - 1 - i],
                    (Ident){
                        .cstr = COMPILER_STRING(
                            compiler,
                            tmp_ident,
                            "->",
                            clsdef->sig.params[clsdef->sig.params_count - 1 - i].data
                        ),
                    },
                    clsdef->sig.defaults[clsdef->sig.defaults_count - 1 - i]
                )
            );
        }
        // TODO: shouldn't have to declare a variable for this
        const char* id = generate_ident(compiler);
        Instruction init_call = {
            .kind = INST_ASSIGNMENT,
            .assignment.type_info = NONE_TYPE,
            .assignment.left.cstr = id,
        };
        render_object_method_call(
            compiler, &init_call.assignment, tmp_self.assignment, init_def, args
        );
        add_instruction(
            compiler,
            (Instruction){
                .kind = INST_DECLARE_VARIABLE,
                .declare_variable.ident.cstr = id,
                .declare_variable.type_info = NONE_TYPE,
            }
        );
        add_instruction(compiler, init_call);
        return;
    }

    // render members to variables
    const char* variable_idents[clsdef->sig.params_count];
    render_callable_args_to_variables(
        compiler, args, clsdef->sig, clsdef->name.data, variable_idents
    );
    // set member to variable value
    for (size_t i = 0; i < clsdef->sig.params_count; i++) {
        add_instruction(
            compiler,
            (Instruction){
                .kind = INST_ASSIGNMENT,
                .assignment.left.cstr = COMPILER_STRING(
                    compiler, tmp_ident, "->", clsdef->sig.params[i].data
                ),
                .assignment.right = variable_idents[i],
                .assignment.type_info = clsdef->sig.types[i],
            }
        );
    }
}

static void
render_call_operation(
    C_Compiler* compiler,
    AssignmentInst* assignment,
    const char* fn_ident,
    Signature sig,
    Arguments* args
)
{
    // TODO: test calling things that aren't callable

    size_t argc = 1 + sig.params_count;
    const char* arg_idents[argc];
    // TODO: once functions have names stored on their struct the correct name could
    // be passed to this function
    render_callable_args_to_variables(compiler, args, sig, NULL, arg_idents + 1);
    arg_idents[0] = COMPILER_STRING(compiler, fn_ident, ".ctx");

    const char* fn_ptr = sb_c_cast(
        &compiler->sb,
        COMPILER_STRING(compiler, fn_ident, ".addr"),
        (TypeInfo){.type = NPTYPE_FUNCTION, .sig = &sig}
    );

    // write the call statement
    check_assignment_type_info(compiler, assignment, sig.return_type);
    assignment->right = c_function_calln(compiler, fn_ptr, arg_idents, argc);
}

static TypeInfo
get_class_member_type_info(
    C_Compiler* compiler, ClassStatement* clsdef, SourceString member_name
)
{
    Symbol* sym = symbol_hm_get(&clsdef->scope->hm, member_name);
    if (!sym) goto error;
    switch (sym->kind) {
        case SYM_MEMBER:
            return *sym->member_type;
        case SYM_FUNCTION:
            return (TypeInfo){.type = NPTYPE_FUNCTION, .sig = &sym->func->sig};
        default:
            goto error;
    }
error:
    name_errorf(
        compiler->file_index,
        compiler->current_stmt_location,
        "unknown member `%s` for type `%s`",
        member_name.data,
        clsdef->name.data
    );
    UNREACHABLE();
}

static void
render_getattr_operation(
    C_Compiler* compiler,
    AssignmentInst* assignment,
    const char* left_repr,
    TypeInfo left_type,
    SourceString attr
)
{
    // TODO: refactor list/dict to use this system once functions have been completely
    // implemented
    if (left_type.type != NPTYPE_OBJECT) {
        type_error(
            compiler->file_index,
            compiler->current_operation_location,
            "getattr operation is currently only implemented for `object` types"
        );
    }

    TypeInfo attr_type = get_class_member_type_info(compiler, left_type.cls, attr);
    check_assignment_type_info(compiler, assignment, attr_type);

    if (attr_type.type == NPTYPE_FUNCTION) {
        // TODO: make sure `self` isn't being bound both here and at the call site
        Symbol* sym = symbol_hm_get(&left_type.cls->scope->hm, attr);
        const char* tmp_ident = generate_ident(compiler);
        add_instruction(
            compiler,
            (Instruction){
                .kind = INST_DECLARE_VARIABLE,
                .declare_variable.ident.cstr = tmp_ident,
                .declare_variable.type_info = attr_type}
        );
        add_instruction(
            compiler,
            (Instruction){
                .kind = INST_ASSIGNMENT,
                .assignment.left.cstr = tmp_ident,
                .assignment.right = sym->func->ns_ident.data,
                .assignment.type_info = left_type}
        );
        add_instruction(
            compiler,
            (Instruction){
                .kind = INST_ASSIGNMENT,
                .assignment.left.cstr = COMPILER_STRING(compiler, tmp_ident, ".ctx.self"),
                .assignment.right = left_repr,
                .assignment.type_info = left_type}
        );
        assignment->right = tmp_ident;
    }
    else
        assignment->right = COMPILER_STRING(compiler, left_repr, "->", attr.data);
}

static void
render_call_from_operands(
    C_Compiler* compiler, AssignmentInst* assignment, Operand left, Operand right
)
{
    Symbol* sym = get_symbol(compiler, left.token.value);
    if (!sym) {
        render_builtin(compiler, assignment, left.token.value.data, right.args);
        return;
    }

    switch (sym->kind) {
        case SYM_CLASS:
            render_object_creation(compiler, assignment, sym->cls, right.args);
            return;
        case SYM_GLOBAL:
            if (sym->globalvar->type_info.type != NPTYPE_FUNCTION)
                type_errorf(
                    compiler->file_index,
                    compiler->current_operation_location,
                    "symbol `%s` is not callable",
                    sym->identifier.data
                );
            render_call_operation(
                compiler,
                assignment,
                sym->globalvar->compiled_name.data,
                *sym->globalvar->type_info.sig,
                right.args
            );
            return;
        case SYM_VARIABLE:
            if (sym->variable->type_info.type != NPTYPE_FUNCTION)
                type_errorf(
                    compiler->file_index,
                    compiler->current_operation_location,
                    "symbol `%s` is not callable",
                    sym->identifier.data
                );
            render_call_operation(
                compiler,
                assignment,
                sym->variable->compiled_name.data,
                *sym->variable->type_info.sig,
                right.args
            );
            return;
        case SYM_FUNCTION:
            render_call_operation(
                compiler, assignment, sym->func->ns_ident.data, sym->func->sig, right.args
            );
            return;
        case SYM_MEMBER:
            UNREACHABLE();
    }
}

#define IS_SIMPLE_EXPRESSION(expr) ((expr)->operations_count <= 1)

static void
render_simple_expression(
    C_Compiler* compiler, AssignmentInst* assignment, Expression* expr
)
{
    if (expr->operations_count == 0) {
        Operand operand = expr->operands[0];
        render_operand(compiler, assignment, operand);
        return;
    }
    else if (expr->operations_count == 1) {
        Operation operation = expr->operations[0];
        compiler->current_operation_location = *operation.loc;

        if (operation.op_type == OPERATOR_CALL) {
            Operand left = expr->operands[operation.left];
            Operand right = expr->operands[operation.right];
            render_call_from_operands(compiler, assignment, left, right);
            return;
        }
        if (operation.op_type == OPERATOR_GET_ATTR) {
            const char* left_repr =
                token_operand_repr(compiler, expr->operands[operation.left]);
            TypeInfo left_type =
                resolve_operand_type(&compiler->tc, expr->operands[operation.left]);
            render_getattr_operation(
                compiler,
                assignment,
                left_repr,
                left_type,
                expr->operands[operation.right].token.value
            );
            return;
        }

        bool is_unary =
            (operation.op_type == OPERATOR_LOGICAL_NOT ||
             operation.op_type == OPERATOR_NEGATIVE ||
             operation.op_type == OPERATOR_BITWISE_NOT);

        Operand operands[2] = {
            expr->operands[operation.left], expr->operands[operation.right]};
        TypeInfo operand_types[2] = {0};
        const char* operand_reprs[2];

        for (size_t lr = (is_unary) ? 1 : 0; lr < 2; lr++) {
            if (operands[lr].kind == OPERAND_TOKEN) {
                operand_reprs[lr] = token_operand_repr(compiler, operands[lr]);
                operand_types[lr] = resolve_operand_type(&compiler->tc, operands[lr]);
            }
            else {
                const char* operand_ident = generate_ident(compiler);
                Instruction operand_assignment_inst = {
                    .kind = INST_ASSIGNMENT, .assignment.left.cstr = operand_ident};
                render_operand(
                    compiler, &operand_assignment_inst.assignment, operands[lr]
                );

                add_instruction(
                    compiler,
                    (Instruction){
                        .kind = INST_DECLARE_VARIABLE,
                        .declare_variable.ident.cstr = operand_ident,
                        .declare_variable.type_info =
                            operand_assignment_inst.assignment.type_info}
                );
                add_instruction(compiler, operand_assignment_inst);

                operand_reprs[lr] = operand_assignment_inst.assignment.left.cstr;
                operand_types[lr] = operand_assignment_inst.assignment.type_info;
            }
        }
        render_operation(
            compiler, assignment, operation.op_type, operand_reprs, operand_types
        );
        return;
    }
    UNREACHABLE();
}

typedef struct {
    AssignmentInst* assignments;
    AssignmentInst** operand_index_to_previous_assignment;
    size_t current_assignment;
} IntermediateOperationRecord;

#define INIT_INTERMEDIATE_OPERATION_RECORD(varname, expression_ptr)                      \
    AssignmentInst completed_assignments_memory[(expression_ptr)->operations_count - 1]; \
    AssignmentInst* lookup_by_operand_memory[(expression_ptr)->operands_count];          \
    memset(                                                                              \
        completed_assignments_memory,                                                    \
        0,                                                                               \
        sizeof(AssignmentInst) * ((expression_ptr)->operations_count - 1)                \
    );                                                                                   \
    memset(                                                                              \
        lookup_by_operand_memory,                                                        \
        0,                                                                               \
        sizeof(AssignmentInst*) * ((expression_ptr)->operands_count)                     \
    );                                                                                   \
    IntermediateOperationRecord varname = {                                              \
        .assignments = completed_assignments_memory,                                     \
        .operand_index_to_previous_assignment = lookup_by_operand_memory,                \
        .current_assignment = 0}

static void
update_intermediate_operation_record(
    IntermediateOperationRecord* mem, AssignmentInst* current, Operation operation
)
{
    bool is_unary =
        (operation.op_type == OPERATOR_LOGICAL_NOT ||
         operation.op_type == OPERATOR_NEGATIVE ||
         operation.op_type == OPERATOR_BITWISE_NOT);

    AssignmentInst** previously_rendered;
    mem->assignments[mem->current_assignment] = *current;
    current = mem->assignments + mem->current_assignment++;

    if (!is_unary) {
        if ((previously_rendered =
                 mem->operand_index_to_previous_assignment + operation.left)) {
            *previously_rendered = current;
        }
        else
            mem->operand_index_to_previous_assignment[operation.left] = current;
    }

    if ((previously_rendered =
             mem->operand_index_to_previous_assignment + operation.right)) {
        *previously_rendered = current;
    }
    else
        mem->operand_index_to_previous_assignment[operation.right] = current;
}

#define IS_NULL_IDENT(ident) ((ident).kind == IDENT_CSTR && !((ident).cstr))

Instruction
expr_to_assignment_inst(
    C_Compiler* compiler, TypeInfo info, Ident identifier, Expression* expr
)
{
    bool should_declare_variable = false;
    if (IS_NULL_IDENT(identifier)) {
        identifier.cstr = generate_ident(compiler);
        should_declare_variable = true;
    }
    Instruction inst = {
        .kind = INST_ASSIGNMENT,
        .assignment.type_info = info,
        .assignment.left = identifier};

    if (IS_SIMPLE_EXPRESSION(expr)) {
        render_simple_expression(compiler, &inst.assignment, expr);
        if (should_declare_variable) {
            add_instruction(
                compiler,
                (Instruction){
                    .kind = INST_DECLARE_VARIABLE,
                    .declare_variable.ident = inst.assignment.left,
                    .declare_variable.type_info = inst.assignment.type_info}
            );
        }
        return inst;
    }

    INIT_INTERMEDIATE_OPERATION_RECORD(record, expr);

    for (size_t i = 0; i < expr->operations_count; i++) {
        Operation operation = expr->operations[i];
        compiler->current_operation_location = *operation.loc;
        Instruction current_inst;

        bool final_operation = false;

        if (i == expr->operations_count - 1) {
            final_operation = true;
            current_inst = inst;
        }
        else {
            current_inst = (Instruction){
                .kind = INST_ASSIGNMENT,
                .assignment.left.cstr = generate_ident(compiler),
            };
        }

        switch (operation.op_type) {
            case OPERATOR_CALL: {
                Operand right = expr->operands[operation.right];

                AssignmentInst* previous =
                    record.operand_index_to_previous_assignment[operation.left];
                if (previous) {
                    render_call_operation(
                        compiler,
                        &current_inst.assignment,
                        ident(previous->left),
                        *previous->type_info.sig,
                        right.args
                    );
                }
                else {
                    Operand left = expr->operands[operation.left];
                    render_call_from_operands(
                        compiler, &current_inst.assignment, left, right
                    );
                }
            } break;
            case OPERATOR_GET_ATTR: {
                AssignmentInst left_assignment = {0};
                AssignmentInst* previous =
                    record.operand_index_to_previous_assignment[operation.left];
                if (previous)
                    left_assignment = *previous;
                else {
                    Instruction left_inst = {
                        .kind = INST_ASSIGNMENT,
                        .assignment.left.cstr = generate_ident(compiler),
                    };
                    Operand operand = expr->operands[operation.left];
                    render_operand(compiler, &left_inst.assignment, operand);
                    add_instruction(
                        compiler,
                        (Instruction){
                            .kind = INST_DECLARE_VARIABLE,
                            .declare_variable.ident = left_inst.assignment.left,
                            .declare_variable.type_info = left_inst.assignment.type_info,
                        }
                    );
                    add_instruction(compiler, left_inst);
                    left_assignment = left_inst.assignment;
                }

                switch (left_assignment.type_info.type) {
                    case NPTYPE_LIST: {
                        if (i == expr->operations_count - 1) {
                            type_error(
                                compiler->file_index,
                                compiler->current_operation_location,
                                "builtin list methods cannot be referenced"
                            );
                        }
                        final_operation = ++i == expr->operations_count - 1;
                        if (final_operation) current_inst = inst;
                        Operation next_operation = expr->operations[i];
                        if (next_operation.op_type != OPERATOR_CALL) {
                            syntax_error(
                                compiler->file_index,
                                *next_operation.loc,
                                0,
                                "expecting function call"
                            );
                        }
                        compiler->current_operation_location = *next_operation.loc;
                        render_list_builtin(
                            compiler,
                            &current_inst.assignment,
                            left_assignment,
                            expr->operands[operation.right].token.value.data,
                            expr->operands[next_operation.right].args
                        );
                        update_intermediate_operation_record(
                            &record, &current_inst.assignment, next_operation
                        );
                    } break;
                    case NPTYPE_DICT: {
                        if (i == expr->operations_count - 1) {
                            type_error(
                                compiler->file_index,
                                compiler->current_operation_location,
                                "builtin dict methods cannot be referenced"
                            );
                        }
                        final_operation = ++i == expr->operations_count - 1;
                        if (final_operation) current_inst = inst;
                        Operation next_operation = expr->operations[i];
                        if (next_operation.op_type != OPERATOR_CALL) {
                            syntax_error(
                                compiler->file_index,
                                *next_operation.loc,
                                0,
                                "expecting function call"
                            );
                        }
                        compiler->current_operation_location = *next_operation.loc;
                        compile_dict_builtin(
                            compiler,
                            &current_inst.assignment,
                            left_assignment,
                            expr->operands[operation.right].token.value.data,
                            expr->operands[next_operation.right].args
                        );
                        update_intermediate_operation_record(
                            &record, &current_inst.assignment, next_operation
                        );
                    } break;
                    case NPTYPE_OBJECT: {
                        render_getattr_operation(
                            compiler,
                            &current_inst.assignment,
                            ident(left_assignment.left),
                            left_assignment.type_info,
                            expr->operands[operation.right].token.value
                        );
                    } break;
                    default:
                        UNIMPLEMENTED(
                            "getattr only currently implemented on lists dicts and "
                            "objects"
                        );
                }
            } break;
            default: {
                size_t operand_indices[2] = {operation.left, operation.right};
                TypeInfo operand_types[2] = {0};
                const char* operand_reprs[2] = {0};

                bool is_unary =
                    (operation.op_type == OPERATOR_LOGICAL_NOT ||
                     operation.op_type == OPERATOR_NEGATIVE ||
                     operation.op_type == OPERATOR_BITWISE_NOT);

                for (size_t lr = (is_unary) ? 1 : 0; lr < 2; lr++) {
                    AssignmentInst* previously_rendered =
                        record.operand_index_to_previous_assignment[operand_indices[lr]];
                    if (previously_rendered) {
                        operand_types[lr] = previously_rendered->type_info;
                        operand_reprs[lr] = ident(previously_rendered->left);
                        continue;
                    }

                    Operand operand = expr->operands[operand_indices[lr]];
                    if (operand.kind == OPERAND_TOKEN) {
                        operand_reprs[lr] = token_operand_repr(compiler, operand);
                        operand_types[lr] = resolve_operand_type(&compiler->tc, operand);
                    }
                    else {
                        Instruction operand_inst = {
                            .kind = INST_ASSIGNMENT,
                            .assignment.left.cstr = generate_ident(compiler)};
                        render_operand(compiler, &operand_inst.assignment, operand);
                        add_instruction(
                            compiler,
                            (Instruction){
                                .kind = INST_DECLARE_VARIABLE,
                                .declare_variable.ident = operand_inst.assignment.left,
                                .declare_variable.type_info =
                                    operand_inst.assignment.type_info,
                            }
                        );
                        add_instruction(compiler, operand_inst);
                        operand_reprs[lr] = operand_inst.assignment.left.cstr;
                        operand_types[lr] = operand_inst.assignment.type_info;
                    }
                }

                render_operation(
                    compiler,
                    &current_inst.assignment,
                    operation.op_type,
                    operand_reprs,
                    operand_types
                );
            }
        }

        if (final_operation) {
            inst = current_inst;
            break;
        }

        update_intermediate_operation_record(
            &record, &current_inst.assignment, operation
        );
        add_instruction(
            compiler,
            (Instruction){
                .kind = INST_DECLARE_VARIABLE,
                .declare_variable.ident = current_inst.assignment.left,
                .declare_variable.type_info = current_inst.assignment.type_info,
            }
        );
        add_instruction(compiler, current_inst);
    }

    if (should_declare_variable) {
        add_instruction(
            compiler,
            (Instruction){
                .kind = INST_DECLARE_VARIABLE,
                .declare_variable.ident = inst.assignment.left,
                .declare_variable.type_info = inst.assignment.type_info}
        );
    }
    return inst;
}

#define COMPILER_ACCUMULATE_INSTRUCTIONS(compiler, comp_inst)                            \
    CompoundInstruction MACRO_VAR(current) = compiler->instructions;                     \
    for (int _i_ =                                                                       \
             (compiler->instructions = compound_instruction_init(compiler->arena), 0);   \
         _i_ == 0;                                                                       \
         _i_ =                                                                           \
             ((comp_inst) = compiler->instructions,                                      \
             compiler->instructions = MACRO_VAR(current),                                \
             compound_instruction_finalize(&(comp_inst)),                                \
             1))

static void
compile_function(C_Compiler* compiler, FunctionStatement* func)
{
    scope_stack_push(&compiler->scope_stack, func->scope);

    const char* fn_ident = generate_ident(compiler);
    Instruction* inst = add_instruction(
        compiler,
        (Instruction){
            .kind = INST_DEFINE_FUNCTION,
            .define_function.function_name = fn_ident,
            .define_function.signature = func->sig,
            .define_function.public_variable =
                (DeclareVariableInst){
                    .type_info = (TypeInfo){.type = NPTYPE_FUNCTION, .sig = &func->sig},
                    .ident.cstr = func->ns_ident.data,
                },
            .define_function.assign_fn_variable =
                (AssignmentInst){
                    .left.cstr = COMPILER_STRING(compiler, func->ns_ident.data, ".addr"),
                    .right = REFERENCE(compiler, fn_ident),
                },
        }
    );

    COMPILER_ACCUMULATE_INSTRUCTIONS(compiler, inst->define_function.body)
    {
        declare_scope_variables(compiler, func->scope);
        if (func->self_param.data) {
            // set `self` variable from context
            add_instruction(
                compiler,
                (Instruction){
                    .kind = INST_ASSIGNMENT,
                    .assignment.left.cstr = func->self_param.data,
                    .assignment.right =
                        sb_c_cast(&compiler->sb, "__ctx__.self", func->self_type),
                }
            );
        }

        for (size_t i = 0; i < func->body.stmts_count; i++)
            compile_statement(compiler, func->body.stmts[i]);

        if (func->sig.return_type.type == NPTYPE_NONE)
            add_instruction(
                compiler,
                (Instruction){
                    .kind = INST_EXPRESSION,
                    .expression = COMPILER_STRING(compiler, "return NULL"),
                }
            );
    }

    scope_stack_pop(&compiler->scope_stack);
}

static void
compile_class(C_Compiler* compiler, ClassStatement* cls)
{
    scope_stack_push(&compiler->scope_stack, cls->scope);

    if (cls->sig.params_count == 0) {
        unspecified_error(
            compiler->file_index,
            compiler->current_stmt_location,
            "class defined without any annotated members"
        );
    }

    Instruction* inst = add_instruction(
        compiler,
        (Instruction){
            .kind = INST_DEFINE_CLASS,
            .define_class.class_name = cls->ns_ident.data,
            .define_class.signature = cls->sig,
        }
    );
    COMPILER_ACCUMULATE_INSTRUCTIONS(compiler, inst->define_class.body)
    {
        for (size_t i = 0; i < cls->body.stmts_count; i++) {
            Statement* stmt = cls->body.stmts[i];
            switch (stmt->kind) {
                case STMT_FUNCTION:
                    compile_function(compiler, stmt->func);
                case STMT_ANNOTATION:
                    break;
                case NULL_STMT:
                    break;
                case STMT_EOF:
                    break;
                default:
                    unspecified_error(
                        compiler->file_index,
                        stmt->loc,
                        "only function definitions and annotations are currently "
                        "implemented "
                        "within the body of a class"
                    );
            }
        }
    }
}

static void
render_list_set_item(
    C_Compiler* compiler,
    AssignmentInst list_assignment,
    const char* idx_variable,
    const char* val_variable
)
{
    add_instruction(
        compiler,
        (Instruction){
            .kind = INST_EXPRESSION,
            .expression = c_function_call(
                compiler,
                NPLIB_LIST_SET_ITEM,
                ident(list_assignment.left),
                idx_variable,
                REFERENCE(compiler, val_variable),
                NULL
            ),
        }
    );
}

static void
render_dict_set_item(
    C_Compiler* compiler,
    AssignmentInst dict_assignment,
    const char* key_variable,
    const char* val_variable
)
{
    add_instruction(
        compiler,
        (Instruction){
            .kind = INST_EXPRESSION,
            .expression = c_function_call(
                compiler,
                NPLIB_DICT_SET_ITEM,
                ident(dict_assignment.left),
                REFERENCE(compiler, key_variable),
                REFERENCE(compiler, val_variable),
                NULL
            ),
        }
    );
}

static Operator OP_ASSIGNMENT_TO_OP_TABLE[OPERATORS_MAX] = {
    [OPERATOR_PLUS_ASSIGNMENT] = OPERATOR_PLUS,
    [OPERATOR_MINUS_ASSIGNMENT] = OPERATOR_MINUS,
    [OPERATOR_MULT_ASSIGNMENT] = OPERATOR_MULT,
    [OPERATOR_DIV_ASSIGNMENT] = OPERATOR_DIV,
    [OPERATOR_MOD_ASSIGNMENT] = OPERATOR_MOD,
    [OPERATOR_FLOORDIV_ASSIGNMENT] = OPERATOR_FLOORDIV,
    [OPERATOR_POW_ASSIGNMENT] = OPERATOR_POW,
    [OPERATOR_AND_ASSIGNMENT] = OPERATOR_BITWISE_AND,
    [OPERATOR_OR_ASSIGNMENT] = OPERATOR_BITWISE_OR,
    [OPERATOR_XOR_ASSIGNMENT] = OPERATOR_BITWISE_XOR,
    [OPERATOR_RSHIFT_ASSIGNMENT] = OPERATOR_RSHIFT,
    [OPERATOR_LSHIFT_ASSIGNMENT] = OPERATOR_LSHIFT,
};

static void
compile_object_op_assignment(
    C_Compiler* compiler,
    AssignmentInst obj_assignment,
    AssignmentInst other_assignment,
    Operator op_assignment_type
)
{
    ClassStatement* clsdef = obj_assignment.type_info.cls;
    ObjectModel om = op_assignment_to_object_model(op_assignment_type);
    FunctionStatement* fndef = clsdef->object_model_methods[om];

    // handle errors
    if (!fndef) {
        type_errorf(
            compiler->file_index,
            compiler->current_stmt_location,
            "type `%s` does not support `%s` operation",
            errfmt_type_info(obj_assignment.type_info),
            op_to_cstr(op_assignment_type)
        );
    }
    if (!compare_types(other_assignment.type_info, fndef->sig.types[0])) {
        type_errorf(
            compiler->file_index,
            compiler->current_stmt_location,
            "type `%s` does not support `%s` operation with `%s` rtype",
            errfmt_type_info(obj_assignment.type_info),
            op_to_cstr(op_assignment_type),
            errfmt_type_info(other_assignment.type_info)
        );
    }

    // create a context containing `self`:
    Instruction ctx_inst = {
        .kind = INST_DECLARE_VARIABLE,
        .declare_variable.ident.cstr = generate_ident(compiler),
        .declare_variable.type_info.type = NPTYPE_CONTEXT,
    };
    add_instruction(compiler, ctx_inst);
    add_instruction(
        compiler,
        (Instruction){
            .kind = INST_ASSIGNMENT,
            .assignment.type_info.type = NPTYPE_CONTEXT,
            .assignment.left.cstr = ctx_inst.declare_variable.ident.cstr,
            .assignment.right =
                sb_build_cstr(&compiler->sb, fndef->ns_ident.data, ".ctx", NULL),
        }
    );
    add_instruction(
        compiler,
        (Instruction){
            .kind = INST_ASSIGNMENT,
            .assignment.type_info = obj_assignment.type_info,
            .assignment.left.cstr = sb_build_cstr(
                &compiler->sb, ctx_inst.declare_variable.ident.cstr, ".self", NULL
            ),
            .assignment.right = ident(obj_assignment.left),
        }
    );

    // cast NpFunction variable addr member to proper c function type
    const char* fn_ptr = sb_c_cast(
        &compiler->sb,
        COMPILER_STRING(compiler, fndef->ns_ident.data, ".addr"),
        (TypeInfo){.type = NPTYPE_FUNCTION, .sig = &fndef->sig}
    );
    add_instruction(
        compiler,
        (Instruction){
            .kind = INST_ASSIGNMENT,
            .assignment.left = obj_assignment.left,
            .assignment.right = c_function_call(
                compiler, fn_ptr, inst_ident(ctx_inst), ident(other_assignment.left), NULL
            ),
        }
    );
}

static void
compile_complex_assignment(C_Compiler* compiler, Statement* stmt)
{
    // TODO: in the future I might want to try and break up this function
    // it was broken up in the past but was quite awkward as such.
    //
    Operation last_op = stmt->assignment->storage
                            ->operations[stmt->assignment->storage->operations_count - 1];
    Operand last_operand = stmt->assignment->storage->operands[last_op.right];

    // render all but the last operation of left hand side to a variable
    //      ex: a.b.c.d              would render a.b.c to a variable
    //      ex: [1, 2, 3].copy()     would render [1,2,3] to a variable
    Expression container_expr = *stmt->assignment->storage;
    container_expr.operations_count -= 1;

    Instruction container_inst =
        expr_to_assignment_inst(compiler, UNTYPED, NULL_IDENT, &container_expr);
    add_instruction(compiler, container_inst);

    if (last_op.op_type == OPERATOR_GET_ITEM) {
        // set key/val expected types
        TypeInfo key_type_info;
        TypeInfo val_type_info;
        switch (container_inst.assignment.type_info.type) {
            case NPTYPE_LIST: {
                key_type_info.type = NPTYPE_INT;
                val_type_info = container_inst.assignment.type_info.inner->types[0];
                break;
            }
            case NPTYPE_DICT: {
                key_type_info = container_inst.assignment.type_info.inner->types[0];
                val_type_info = container_inst.assignment.type_info.inner->types[1];
                break;
            }
            default:
                UNIMPLEMENTED("setitem not implemented for this data type");
        }

        // render key to a variable
        Instruction key_inst = {
            .kind = INST_ASSIGNMENT,
            .assignment.left.cstr = generate_ident(compiler),
            .assignment.type_info = key_type_info};
        render_operand(compiler, &key_inst.assignment, last_operand);
        add_instruction(
            compiler,
            (Instruction){
                .kind = INST_DECLARE_VARIABLE,
                .declare_variable.ident = key_inst.assignment.left,
                .declare_variable.type_info = key_inst.assignment.type_info,
            }
        );
        add_instruction(compiler, key_inst);

        Instruction val_inst;
        // render value to a variable
        //
        // regular assignment
        if (stmt->assignment->op_type == OPERATOR_ASSIGNMENT) {
            val_inst = expr_to_assignment_inst(
                compiler, val_type_info, NULL_IDENT, stmt->assignment->value
            );
            add_instruction(compiler, val_inst);
        }
        // op-assignment
        else {
            // getitem
            Instruction current_val_inst = {
                .kind = INST_ASSIGNMENT,
                .assignment.left.cstr = generate_ident(compiler),
                .assignment.type_info = val_type_info};
            const char* current_reprs[2] = {
                ident(container_inst.assignment.left), ident(key_inst.assignment.left)};
            TypeInfo current_types[2] = {
                container_inst.assignment.type_info, key_inst.assignment.type_info};
            render_operation(
                compiler,
                &current_val_inst.assignment,
                OPERATOR_GET_ITEM,
                current_reprs,
                current_types
            );
            add_instruction(
                compiler,
                (Instruction){
                    .kind = INST_DECLARE_VARIABLE,
                    .declare_variable.ident = current_val_inst.assignment.left,
                    .declare_variable.type_info = current_val_inst.assignment.type_info,
                }
            );
            add_instruction(compiler, current_val_inst);

            // right side expression
            Instruction other_val_inst = expr_to_assignment_inst(
                compiler, UNTYPED, NULL_IDENT, stmt->assignment->value
            );
            add_instruction(compiler, other_val_inst);

            // render __iadd__, __isub__... object model method
            if (current_val_inst.assignment.type_info.type == NPTYPE_OBJECT) {
                compile_object_op_assignment(
                    compiler,
                    current_val_inst.assignment,
                    other_val_inst.assignment,
                    stmt->assignment->op_type
                );
                return;
            }

            // combine current value with right side expression
            Operator op_type = OP_ASSIGNMENT_TO_OP_TABLE[stmt->assignment->op_type];
            const char* new_reprs[2] = {
                ident(current_val_inst.assignment.left),
                ident(other_val_inst.assignment.left)};
            TypeInfo new_types[2] = {
                current_val_inst.assignment.type_info,
                other_val_inst.assignment.type_info};
            val_inst = (Instruction){
                .kind = INST_ASSIGNMENT,
                .assignment.left.cstr = generate_ident(compiler),
                .assignment.type_info = val_type_info};
            render_operation(
                compiler, &val_inst.assignment, op_type, new_reprs, new_types
            );
            add_instruction(
                compiler,
                (Instruction){
                    .kind = INST_DECLARE_VARIABLE,
                    .declare_variable.ident = val_inst.assignment.left,
                    .declare_variable.type_info = val_inst.assignment.type_info,
                }
            );
            add_instruction(compiler, val_inst);
        }

        switch (container_inst.assignment.type_info.type) {
            case NPTYPE_LIST: {
                render_list_set_item(
                    compiler,
                    container_inst.assignment,
                    ident(key_inst.assignment.left),
                    ident(val_inst.assignment.left)
                );
                return;
            }
            case NPTYPE_DICT: {
                render_dict_set_item(
                    compiler,
                    container_inst.assignment,
                    ident(key_inst.assignment.left),
                    ident(val_inst.assignment.left)
                );
                return;
            }
            default:
                UNIMPLEMENTED("setitem not implemented for this data type");
        }
    }
    else if (last_op.op_type == OPERATOR_GET_ATTR) {
        if (container_inst.assignment.type_info.type != NPTYPE_OBJECT) {
            UNIMPLEMENTED("setattr for this type not implemented");
        }
        ClassStatement* clsdef = container_inst.assignment.type_info.cls;
        TypeInfo member_type =
            get_class_member_type_info(compiler, clsdef, last_operand.token.value);
        const char* left_ident = COMPILER_STRING(
            compiler,
            ident(container_inst.assignment.left),
            "->",
            last_operand.token.value.data
        );

        if (stmt->assignment->op_type == OPERATOR_ASSIGNMENT) {
            // regular assignment
            add_instruction(
                compiler,
                expr_to_assignment_inst(
                    compiler,
                    member_type,
                    (Ident){.cstr = left_ident},
                    stmt->assignment->value
                )
            );
            return;
        }
        else {
            // op assignment
            Operator op_type = OP_ASSIGNMENT_TO_OP_TABLE[stmt->assignment->op_type];

            // render right hand side to variable
            Instruction other_inst = expr_to_assignment_inst(
                compiler, UNTYPED, NULL_IDENT, stmt->assignment->value
            );
            add_instruction(compiler, other_inst);

            // render op assignment object model method (__iadd__, __isub__, ...)
            if (member_type.type == NPTYPE_OBJECT) {
                compile_object_op_assignment(
                    compiler,
                    (AssignmentInst){.left.cstr = left_ident, .type_info = member_type},
                    other_inst.assignment,
                    stmt->assignment->op_type
                );
                return;
            }
            // set member to current_value (op_type) new value
            else {
                // render getattr
                Instruction current_val_inst = {
                    .kind = INST_ASSIGNMENT,
                    .assignment.left.cstr = generate_ident(compiler),
                    .assignment.type_info = member_type};
                render_getattr_operation(
                    compiler,
                    &current_val_inst.assignment,
                    ident(container_inst.assignment.left),
                    container_inst.assignment.type_info,
                    last_operand.token.value
                );
                add_instruction(
                    compiler,
                    (Instruction){
                        .kind = INST_DECLARE_VARIABLE,
                        .declare_variable.ident = current_val_inst.assignment.left,
                        .declare_variable.type_info =
                            current_val_inst.assignment.type_info,
                    }
                );
                add_instruction(compiler, current_val_inst);

                // render operation to combine current value with new expression
                Instruction inst = {
                    .kind = INST_ASSIGNMENT,
                    .assignment.left.cstr = left_ident,
                    .assignment.type_info = member_type};
                const char* reprs[2] = {
                    ident(current_val_inst.assignment.left),
                    ident(other_inst.assignment.left)};
                TypeInfo types[2] = {
                    current_val_inst.assignment.type_info,
                    other_inst.assignment.type_info};
                render_operation(compiler, &inst.assignment, op_type, reprs, types);
                add_instruction(compiler, inst);
            }
        }
        return;
    }
    else
        UNIMPLEMENTED("complex assignment compilation unimplemented for op type");
}

static void
compile_simple_assignment(C_Compiler* compiler, Statement* stmt)
{
    SourceString identifier = stmt->assignment->storage->operands[0].token.value;
    Symbol* sym = get_symbol(compiler, identifier);
    if (!sym)
        name_errorf(
            compiler->file_index, stmt->loc, "undefined symbol `%s`", identifier.data
        );
    add_instruction(
        compiler,
        expr_to_assignment_inst(
            compiler,
            sym->variable->type_info,
            (Ident){.kind = IDENT_VAR, .var = sym->variable},
            stmt->assignment->value
        )
    );
}

static void
compile_simple_op_assignment(C_Compiler* compiler, Statement* stmt)
{
    SourceString identifier = stmt->assignment->storage->operands[0].token.value;
    Symbol* sym = get_symbol(compiler, identifier);
    if (!sym)
        name_errorf(
            compiler->file_index, stmt->loc, "undefined symbol `%s`", identifier.data
        );
    Operator op_type = OP_ASSIGNMENT_TO_OP_TABLE[stmt->assignment->op_type];

    Instruction main_inst = {
        .kind = INST_ASSIGNMENT,
        .assignment.left.kind = IDENT_VAR,
        .assignment.left.var = sym->variable,
        .assignment.type_info = sym->variable->type_info};

    Instruction other_inst;
    add_instruction(
        compiler,
        other_inst = expr_to_assignment_inst(
            compiler, UNTYPED, NULL_IDENT, stmt->assignment->value
        )
    );

    if (sym->variable->type_info.type == NPTYPE_OBJECT) {
        compile_object_op_assignment(
            compiler,
            main_inst.assignment,
            other_inst.assignment,
            stmt->assignment->op_type
        );
    }
    else {
        const char* operand_reprs[2] = {
            ident(main_inst.assignment.left), ident(other_inst.assignment.left)};
        TypeInfo operand_types[2] = {
            main_inst.assignment.type_info, other_inst.assignment.type_info};
        render_operation(
            compiler, &main_inst.assignment, op_type, operand_reprs, operand_types
        );
        add_instruction(compiler, main_inst);
    }
}

static void
compile_assignment(C_Compiler* compiler, Statement* stmt)
{
    if (stmt->assignment->storage->operations_count != 0)
        compile_complex_assignment(compiler, stmt);
    else {
        if (stmt->assignment->op_type != OPERATOR_ASSIGNMENT)
            compile_simple_op_assignment(compiler, stmt);
        else
            compile_simple_assignment(compiler, stmt);
    }
}

static void
compile_annotation(C_Compiler* compiler, Statement* stmt)
{
    Symbol* sym = get_symbol(compiler, stmt->annotation->identifier);
    if (!sym)
        name_errorf(
            compiler->file_index,
            stmt->loc,
            "undefined symbol `%s`",
            stmt->annotation->identifier.data
        );

    if (sym->kind != SYM_VARIABLE)
        syntax_error(compiler->file_index, stmt->loc, 0, "unexpected annotation");

    if (stmt->annotation->initial) {
        add_instruction(
            compiler,
            expr_to_assignment_inst(
                compiler,
                stmt->annotation->type,
                (Ident){.kind = IDENT_VAR, .var = sym->variable},
                stmt->annotation->initial
            )
        );
    }
}

static void
compile_return_statement(C_Compiler* compiler, Expression* value)
{
    LexicalScope* scope = scope_stack_peek(&compiler->scope_stack);

    Instruction retval_inst;
    add_instruction(
        compiler,
        retval_inst = expr_to_assignment_inst(
            compiler, scope->func->sig.return_type, NULL_IDENT, value
        )
    );

    if (retval_inst.assignment.type_info.type == NPTYPE_NONE) {
        add_instruction(
            compiler,
            (Instruction){
                .kind = INST_EXPRESSION,
                .expression = "return",
            }
        );
    }
    else {
        add_instruction(
            compiler,
            (Instruction){
                .kind = INST_EXPRESSION,
                .expression = COMPILER_STRING(
                    compiler, "return ", ident(retval_inst.assignment.left)
                )}
        );
    }
}

static void
init_semi_scoped_variable(
    C_Compiler* compiler, SourceString identifier, TypeInfo type_info
)
{
    Symbol* sym = get_symbol(compiler, identifier);
    if (sym->variable->kind != VAR_SEMI_SCOPED) return;
    sym->variable->compiled_name.data = generate_ident(compiler);
    sym->variable->compiled_name.length = strlen(sym->variable->compiled_name.data);
    sym->variable->type_info = type_info;
    sym->variable->directly_in_scope = true;
    add_instruction(
        compiler,
        (Instruction){
            .kind = INST_DECLARE_VARIABLE,
            .declare_variable.ident.cstr = sym->variable->compiled_name.data,
            .declare_variable.type_info = type_info}
    );
}

static TypeInfo
iterable_of_type(C_Compiler* compiler, TypeInfo type)
{
    TypeInfo info = {
        .type = NPTYPE_ITER,
        .inner = arena_alloc(compiler->arena, sizeof(TypeInfoInner))};
    info.inner->types = arena_alloc(compiler->arena, sizeof(TypeInfo));
    info.inner->types[0] = type;
    info.inner->count = 1;
    return info;
}

const char*
to_iter_function_call(
    C_Compiler* compiler, TypeInfo type_info, const char* ident_to_convert
)
{
    switch (type_info.type) {
        case NPTYPE_LIST:
            return c_function_call(compiler, NPLIB_LIST_ITER, ident_to_convert, NULL);
        case NPTYPE_DICT:
            return c_function_call(compiler, NPLIB_DICT_KEYS, ident_to_convert, NULL);
        default:
            UNIMPLEMENTED("can't convert type to iterable");
    }
    UNREACHABLE();
}

static AssignmentInst
convert_assignment_to_iterator(C_Compiler* compiler, AssignmentInst assignment)
{
    Instruction assignment_inst;
    if (assignment.type_info.type == NPTYPE_LIST ||
        assignment.type_info.type == NPTYPE_DICT) {
        TypeInfo iter_type_info =
            iterable_of_type(compiler, assignment.type_info.inner->types[0]);
        const char* iter_ident = generate_ident(compiler);
        add_instruction(
            compiler,
            (Instruction){
                .kind = INST_DECLARE_VARIABLE,
                .declare_variable.ident.cstr = iter_ident,
                .declare_variable.type_info = iter_type_info,
            }
        );
        add_instruction(
            compiler,
            assignment_inst =
                (Instruction){
                    .kind = INST_ASSIGNMENT,
                    .assignment.left.cstr = iter_ident,
                    .assignment.right = to_iter_function_call(
                        compiler, assignment.type_info, ident(assignment.left)
                    ),
                    .assignment.type_info = iter_type_info,
                }
        );
    }
    else if (assignment.type_info.type == NPTYPE_ITER)
        assignment_inst.assignment = assignment;
    else
        type_errorf(
            compiler->file_index,
            compiler->current_stmt_location,
            "iterators currently implemented only for lists and dicts, got `%s`",
            errfmt_type_info(assignment.type_info)
        );

    return assignment_inst.assignment;
}

static void
compile_itid_declarations(C_Compiler* compiler, TypeInfo iterator_type, ItGroup* its)
{
    if ((its->identifiers_count > 1 || its->identifiers[0].kind == IT_GROUP) &&
        iterator_type.inner->types[0].type != NPTYPE_DICT_ITEMS)
        unspecified_error(
            compiler->file_index,
            compiler->current_stmt_location,
            "multiple iterable identifier variables only supported for dict.items"
        );
    if (iterator_type.inner->types[0].type == NPTYPE_DICT_ITEMS) {
        if (its->identifiers[0].kind == IT_GROUP &&
            (its->identifiers[0].group->identifiers_count != 2 ||
             its->identifiers[0].group->identifiers[0].kind != IT_ID ||
             its->identifiers[0].group->identifiers[1].kind != IT_ID)) {
            syntax_error(
                compiler->file_index,
                compiler->current_stmt_location,
                0,
                "expecting 2 values to unpack for dict.items"
            );
        }
        if (its->identifiers_count == 1) {
            init_semi_scoped_variable(
                compiler,
                its->identifiers[0].group->identifiers[0].name,
                iterator_type.inner->types[0].inner->types[0]
            );
            init_semi_scoped_variable(
                compiler,
                its->identifiers[0].group->identifiers[1].name,
                iterator_type.inner->types[0].inner->types[1]
            );
        }
        else {
            init_semi_scoped_variable(
                compiler,
                its->identifiers[0].name,
                iterator_type.inner->types[0].inner->types[0]
            );
            init_semi_scoped_variable(
                compiler,
                its->identifiers[1].name,
                iterator_type.inner->types[0].inner->types[1]
            );
        }
    }
    else {
        init_semi_scoped_variable(
            compiler, its->identifiers[0].name, iterator_type.inner->types[0]
        );
    }
}

static void
compile_for_loop(C_Compiler* compiler, ForLoopStatement* for_loop)
{
    const char* iter_id;
    TypeInfoInner* iterator_inner;

    const char* next_id = generate_ident(compiler);

    Instruction* loop_inst = add_instruction(
        compiler,
        (Instruction){
            .kind = INST_LOOP,
            .loop.after_label = generate_ident(compiler),
            .loop.condition = next_id,
        }
    );

    COMPILER_ACCUMULATE_INSTRUCTIONS(compiler, loop_inst->loop.init)
    {
        // create iterator and assign `iter_id` value
        Instruction container_inst;
        add_instruction(
            compiler,
            container_inst =
                expr_to_assignment_inst(compiler, UNTYPED, NULL_IDENT, for_loop->iterable)
        );
        AssignmentInst iterator_assignment =
            convert_assignment_to_iterator(compiler, container_inst.assignment);
        iter_id = ident(iterator_assignment.left);

        // declare it identifier variables
        iterator_inner = iterator_assignment.type_info.inner;
        compile_itid_declarations(compiler, iterator_assignment.type_info, for_loop->it);

        // declare void* next variable
        add_instruction(
            compiler,
            (Instruction){
                .kind = INST_DECLARE_VARIABLE,
                .declare_variable.ctype = "void*",
                .declare_variable.ident.cstr = next_id,
            }
        );
        // make first next call
        add_instruction(
            compiler,
            (Instruction){
                .kind = INST_ASSIGNMENT,
                .assignment.left.cstr = next_id,
                .assignment.right = c_function_call(
                    compiler,
                    COMPILER_STRING(compiler, iter_id, ".next"),
                    COMPILER_STRING(compiler, iter_id, ".iter"),
                    NULL
                ),
            }
        );
    }

    COMPILER_ACCUMULATE_INSTRUCTIONS(compiler, loop_inst->loop.before)
    {
        // assign values in void* next variable to it id variables

        const char* key_id;
        const char* val_id;

        if (iterator_inner->types[0].type == NPTYPE_DICT_ITEMS) {
            if (for_loop->it->identifiers_count == 1) {
                Symbol* sym = get_symbol(
                    compiler, for_loop->it->identifiers[0].group->identifiers[0].name
                );
                key_id = sym->variable->compiled_name.data;
                sym = get_symbol(
                    compiler, for_loop->it->identifiers[0].group->identifiers[1].name
                );
                val_id = sym->variable->compiled_name.data;
            }
            else {
                Symbol* sym = get_symbol(compiler, for_loop->it->identifiers[0].name);
                key_id = sym->variable->compiled_name.data;
                sym = get_symbol(compiler, for_loop->it->identifiers[1].name);
                val_id = sym->variable->compiled_name.data;
            }

            // cast void* next into DictItem*
            // cast void* DictItem->key into type* and deref
            // *( (type*) (((DictItem*)next)->key) )
            // we know next is not NULL

            add_instruction(
                compiler,
                (Instruction){
                    .kind = INST_ASSIGNMENT,
                    .assignment.left.cstr = key_id,
                    .assignment.right = COMPILER_STRING(
                        compiler,
                        "*((",
                        type_info_to_c_syntax(
                            &compiler->sb, iterator_inner->types[0].inner->types[0]
                        ),
                        "*)(((DictItem*)",
                        next_id,
                        ")->key))"
                    ),
                }
            );
            add_instruction(
                compiler,
                (Instruction){
                    .kind = INST_ASSIGNMENT,
                    .assignment.left.cstr = val_id,
                    .assignment.right = COMPILER_STRING(
                        compiler,
                        "*((",
                        type_info_to_c_syntax(
                            &compiler->sb, iterator_inner->types[0].inner->types[1]
                        ),
                        "*)(((DictItem*)",
                        next_id,
                        ")->val))"
                    )}
            );
        }
        else {
            // cast void* next into type* and deref
            // *( (type*)next )
            Symbol* sym = get_symbol(compiler, for_loop->it->identifiers[0].name);
            const char* id = sym->variable->compiled_name.data;
            add_instruction(
                compiler,
                (Instruction){
                    .kind = INST_ASSIGNMENT,
                    .assignment.left.cstr = id,
                    .assignment.right = COMPILER_STRING(
                        compiler,
                        "*((",
                        type_info_to_c_syntax(&compiler->sb, iterator_inner->types[0]),
                        "*)",
                        next_id,
                        ")"
                    ),
                }
            );
        }
    }

    COMPILER_ACCUMULATE_INSTRUCTIONS(compiler, loop_inst->loop.after)
    {
        // make call to iter.next
        add_instruction(
            compiler,
            (Instruction){.kind = INST_LABEL, .label = loop_inst->loop.after_label}
        );
        add_instruction(
            compiler,
            (Instruction){
                .kind = INST_ASSIGNMENT,
                .assignment.left.cstr = next_id,
                .assignment.right = c_function_call(
                    compiler,
                    COMPILER_STRING(compiler, iter_id, ".next"),
                    COMPILER_STRING(compiler, iter_id, ".iter"),
                    NULL
                ),
            }
        );
    }

    COMPILER_ACCUMULATE_INSTRUCTIONS(compiler, loop_inst->loop.body)
    {
        for (size_t i = 0; i < for_loop->body.stmts_count; i++)
            compile_statement(compiler, for_loop->body.stmts[i]);
    }

    // release semi-scoped variables
    if (iterator_inner->types[0].type == NPTYPE_DICT_ITEMS) {
        if (for_loop->it->identifiers_count == 1) {
            Symbol* sym = get_symbol(
                compiler, for_loop->it->identifiers[0].group->identifiers[0].name
            );
            sym->variable->directly_in_scope = false;
            sym = get_symbol(
                compiler, for_loop->it->identifiers[0].group->identifiers[1].name
            );
            sym->variable->directly_in_scope = false;
        }
        else {
            Symbol* sym = get_symbol(compiler, for_loop->it->identifiers[0].name);
            sym->variable->directly_in_scope = false;
            sym = get_symbol(compiler, for_loop->it->identifiers[1].name);
            sym->variable->directly_in_scope = false;
        }
    }
    else {
        Symbol* sym = get_symbol(compiler, for_loop->it->identifiers[0].name);
        sym->variable->directly_in_scope = false;
    }
}

static void
compile_if(C_Compiler* compiler, ConditionalStatement* conditional)
{
    const char* exit_ident = NULL;
    bool req_goto =
        (conditional->else_body.stmts_count > 0 || conditional->elifs_count > 0);
    if (req_goto) exit_ident = generate_ident(compiler);

    // if
    Instruction condition_inst;

    add_instruction(
        compiler,
        condition_inst =
            expr_to_assignment_inst(compiler, UNTYPED, NULL_IDENT, conditional->condition)
    );
    Instruction* if_inst = add_instruction(
        compiler,
        (Instruction){
            .kind = INST_IF,
            .if_.condition_assignment = condition_inst.assignment,
        }
    );
    COMPILER_ACCUMULATE_INSTRUCTIONS(compiler, if_inst->if_.body)
    {
        for (size_t i = 0; i < conditional->body.stmts_count; i++)
            compile_statement(compiler, conditional->body.stmts[i]);
        if (req_goto)
            add_instruction(
                compiler,
                (Instruction){
                    .kind = INST_GOTO,
                    .label = exit_ident,
                }
            );
    }

    // elifs
    for (size_t i = 0; i < conditional->elifs_count; i++) {
        add_instruction(
            compiler,
            condition_inst = expr_to_assignment_inst(
                compiler, UNTYPED, NULL_IDENT, conditional->elifs[i].condition
            )
        );
        Instruction* if_inst = add_instruction(
            compiler,
            (Instruction){
                .kind = INST_IF,
                .if_.condition_assignment = condition_inst.assignment,
            }
        );
        COMPILER_ACCUMULATE_INSTRUCTIONS(compiler, if_inst->if_.body)
        {
            for (size_t stmt_i = 0; stmt_i < conditional->elifs[i].body.stmts_count;
                 stmt_i++)
                compile_statement(compiler, conditional->elifs[i].body.stmts[stmt_i]);
            if (req_goto)
                add_instruction(
                    compiler,
                    (Instruction){
                        .kind = INST_GOTO,
                        .label = exit_ident,
                    }
                );
        }
    }

    // else
    if (conditional->else_body.stmts_count > 0) {
        Instruction* else_inst =
            add_instruction(compiler, (Instruction){.kind = INST_ELSE});
        COMPILER_ACCUMULATE_INSTRUCTIONS(compiler, else_inst->else_)
        {
            for (size_t i = 0; i < conditional->else_body.stmts_count; i++)
                compile_statement(compiler, conditional->else_body.stmts[i]);
        }
    }

    if (req_goto)
        add_instruction(compiler, (Instruction){.kind = INST_LABEL, .label = exit_ident});
}

static void
compile_while(C_Compiler* compiler, WhileStatement* while_stmt)
{
    Instruction* loop_inst = add_instruction(
        compiler,
        (Instruction){
            .kind = INST_LOOP,
            .loop.condition = "1",
            .loop.after_label = generate_ident(compiler),
        }
    );

    COMPILER_ACCUMULATE_INSTRUCTIONS(compiler, loop_inst->loop.before)
    {
        Instruction condition_inst;

        add_instruction(
            compiler,
            condition_inst = expr_to_assignment_inst(
                compiler, UNTYPED, NULL_IDENT, while_stmt->condition
            )
        );
        Instruction* if_inst = add_instruction(
            compiler,
            (Instruction){
                .kind = INST_IF,
                .if_.negate = true,
                .if_.condition_assignment = condition_inst.assignment,
            }
        );
        COMPILER_ACCUMULATE_INSTRUCTIONS(compiler, if_inst->if_.body)
        {
            add_instruction(compiler, (Instruction){.kind = INST_BREAK});
        }
    }
    COMPILER_ACCUMULATE_INSTRUCTIONS(compiler, loop_inst->loop.body)
    {
        for (size_t i = 0; i < while_stmt->body.stmts_count; i++)
            compile_statement(compiler, while_stmt->body.stmts[i]);
    }
    COMPILER_ACCUMULATE_INSTRUCTIONS(compiler, loop_inst->loop.after)
    {
        add_instruction(
            compiler,
            (Instruction){.kind = INST_LABEL, .label = loop_inst->loop.after_label}
        );
    }
}

static void
check_exceptions(C_Compiler* compiler)
{
    // TODO: replace with callback system in the future
    Instruction* if_inst = add_instruction(
        compiler,
        (Instruction){
            .kind = INST_IF,
            .if_.condition_assignment.left.cstr = "global_exception",
            .if_.condition_assignment.type_info = EXCEPTION_TYPE,
        }
    );
    COMPILER_ACCUMULATE_INSTRUCTIONS(compiler, if_inst->if_.body)
    {
        add_instruction(
            compiler, (Instruction){.kind = INST_GOTO, .label = compiler->excepts_goto}
        );
    }
}

static void
compile_try(C_Compiler* compiler, TryStatement* try)
{
    // TODO: ensure fallthrough to finally
    // TODO: if I supported closures this could somehow be packed very neatly into
    // a callback system that would be convenient because you could easily interact
    // with it from C but for now this seems a lot simpler

    const char* old_goto = compiler->excepts_goto;
    const char* finally_goto = generate_ident(compiler);
    const char* old_excepts_ident = generate_ident(compiler);
    compiler->excepts_goto = generate_ident(compiler);

    add_instruction(
        compiler,
        (Instruction){
            .kind = INST_DECLARE_VARIABLE,
            .declare_variable.ident.cstr = old_excepts_ident,
            .declare_variable.ctype = NPLIB_CURRENT_EXCEPTS_CTYPE}
    );
    add_instruction(
        compiler,
        (Instruction){
            .kind = INST_ASSIGNMENT,
            .assignment.left.cstr = old_excepts_ident,
            .assignment.right = NPLIB_CURRENT_EXCEPTS}
    );

    // | together except types
    size_t excepts_combined = 0;
    sb_begin(&compiler->sb);
    for (size_t i = 0; i < try->excepts_count; i++) {
        for (size_t j = 0; j < try->excepts[i].exceptions_count; j++) {
            if (excepts_combined > 0) sb_add(&compiler->sb, " | ");
            // FIXME:
            // the location here is actually the location of the try statement rather
            // than the except statement -- the parser will need to be updated to
            // capture the location of except statements
            sb_add(
                &compiler->sb,
                source_string_to_exception_type_string(
                    compiler->file_index,
                    compiler->current_stmt_location,
                    try->excepts[i].exceptions[j]
                )
            );
            excepts_combined++;
        }
    }

    add_instruction(
        compiler,
        (Instruction){
            .kind = INST_ASSIGNMENT,
            .assignment.left.cstr = NPLIB_CURRENT_EXCEPTS,
            .assignment.right = sb_end(&compiler->sb),
        }
    );

    // try block
    for (size_t i = 0; i < try->try_body.stmts_count; i++) {
        compile_statement(compiler, try->try_body.stmts[i]);
        // FIXME: once exceptions are handled with closures and a callback system
        // check exceptions wont be necessary
        check_exceptions(compiler);
    }
    Instruction* if_inst = add_instruction(
        compiler,
        (Instruction){
            .kind = INST_IF,
            .if_.condition_assignment.left.cstr = "global_exception",
            .if_.condition_assignment.type_info = EXCEPTION_TYPE,
            .if_.negate = true,
        }
    );
    COMPILER_ACCUMULATE_INSTRUCTIONS(compiler, if_inst->if_.body)
    {
        add_instruction(
            compiler, (Instruction){.kind = INST_GOTO, .label = finally_goto}
        );
    }

    // except blocks
    add_instruction(
        compiler, (Instruction){.kind = INST_LABEL, .label = compiler->excepts_goto}
    );
    const char* exception_id = generate_ident(compiler);
    add_instruction(
        compiler,
        (Instruction){
            .kind = INST_DECLARE_VARIABLE,
            .declare_variable.type_info = EXCEPTION_TYPE,
            .declare_variable.ident.cstr = exception_id,
        }
    );
    add_instruction(
        compiler,
        (Instruction){
            .kind = INST_ASSIGNMENT,
            .assignment.left.cstr = exception_id,
            .assignment.right = c_function_call(compiler, NPLIB_GET_EXCEPTION, NULL),
        }
    );
    for (size_t i = 0; i < try->excepts_count; i++) {
        if (try->excepts[i].as.data)
            UNIMPLEMENTED("capturing exception using `as` keyword is not implemented");

        sb_begin(&compiler->sb);
        sb_add(&compiler->sb, exception_id);
        sb_add(&compiler->sb, "->type & (");

        size_t excepts_combined = 0;
        for (size_t j = 0; j < try->excepts[i].exceptions_count; j++) {
            if (excepts_combined > 0) sb_add(&compiler->sb, " | ");
            // FIXME: imprecise location
            sb_add(
                &compiler->sb,
                source_string_to_exception_type_string(
                    compiler->file_index,
                    compiler->current_stmt_location,
                    try->excepts[i].exceptions[j]
                )
            );
            excepts_combined++;
        }
        sb_add(&compiler->sb, ")");

        Instruction* if_inst = add_instruction(
            compiler,
            (Instruction){
                .kind = INST_IF,
                .if_.condition_assignment.left.cstr = sb_end(&compiler->sb),
                .if_.condition_assignment.type_info = INT_TYPE,
            }
        );
        COMPILER_ACCUMULATE_INSTRUCTIONS(compiler, if_inst->if_.body)
        {
            for (size_t j = 0; j < try->excepts[i].body.stmts_count; j++)
                compile_statement(compiler, try->excepts[i].body.stmts[j]);
            add_instruction(
                compiler, (Instruction){.kind = INST_GOTO, .label = finally_goto}
            );
        }
    }

    // finally
    add_instruction(compiler, (Instruction){.kind = INST_LABEL, .label = finally_goto});
    for (size_t i = 0; i < try->finally_body.stmts_count; i++)
        compile_statement(compiler, try->finally_body.stmts[i]);

    // restore old excepts
    add_instruction(
        compiler,
        (Instruction){
            .kind = INST_ASSIGNMENT,
            .assignment.left.cstr = NPLIB_CURRENT_EXCEPTS,
            .assignment.right = old_excepts_ident}
    );
    compiler->excepts_goto = old_goto;
}

static void
compile_assert(C_Compiler* compiler, Expression* value)
{
    Instruction condition_inst;
    add_instruction(
        compiler,
        condition_inst = expr_to_assignment_inst(compiler, UNTYPED, NULL_IDENT, value)
    );
    Instruction* if_inst = add_instruction(
        compiler,
        (Instruction){
            .kind = INST_IF,
            .if_.condition_assignment = condition_inst.assignment,
            .if_.negate = true,
        }
    );
    COMPILER_ACCUMULATE_INSTRUCTIONS(compiler, if_inst->if_.body)
    {
        add_instruction(
            compiler,
            (Instruction){
                .kind = INST_EXPRESSION,
                .expression = c_function_call(
                    compiler,
                    NPLIB_ASSERTION_ERROR,
                    arena_nfmt(
                        compiler->arena, 16, "%u", compiler->current_stmt_location.line
                    ),
                    NULL
                ),
            }
        );
    }
}

static void
compile_statement(C_Compiler* compiler, Statement* stmt)
{
    compiler->current_stmt_location = stmt->loc;
    compiler->current_operation_location = stmt->loc;

    switch (stmt->kind) {
        case STMT_ASSERT:
            compile_assert(compiler, stmt->assert_expr);
            break;
        case STMT_FOR_LOOP:
            compile_for_loop(compiler, stmt->for_loop);
            break;
        case STMT_IMPORT:
            UNIMPLEMENTED("import compilation is unimplemented");
        case STMT_WHILE:
            compile_while(compiler, stmt->while_loop);
            break;
        case STMT_IF:
            compile_if(compiler, stmt->conditional);
            break;
        case STMT_TRY:
            compile_try(compiler, stmt->try_stmt);
            break;
        case STMT_WITH:
            UNIMPLEMENTED("with compilation is unimplemented");
        case STMT_CLASS:
            compile_class(compiler, stmt->cls);
            break;
        case STMT_FUNCTION:
            compile_function(compiler, stmt->func);
            break;
        case STMT_ASSIGNMENT:
            compile_assignment(compiler, stmt);
            break;
        case STMT_ANNOTATION:
            compile_annotation(compiler, stmt);
            break;
        case STMT_EXPR: {
            add_instruction(
                compiler,
                expr_to_assignment_inst(compiler, UNTYPED, NULL_IDENT, stmt->expr)
            );
            break;
        }
        case STMT_NO_OP:
            // might want to put in a `;` but for now just skipping
            break;
        case STMT_EOF:
            break;
        case STMT_RETURN: {
            // TODO: make sure return statements are only allowed inside functions
            compile_return_statement(compiler, stmt->return_expr);
            break;
        }
        case STMT_BREAK:
            add_instruction(compiler, (Instruction){.kind = INST_BREAK});
            break;
        case STMT_CONTINUE:
            add_instruction(compiler, (Instruction){.kind = INST_CONTINUE});
            break;
        case NULL_STMT:
            break;
        default:
            UNREACHABLE();
    }
}

static void
declare_scope_variables(C_Compiler* compiler, LexicalScope* scope)
{
    for (size_t i = 0; i < scope->hm.elements_count; i++) {
        Symbol s = scope->hm.elements[i];
        if (s.kind == SYM_VARIABLE) {
            switch (s.variable->kind) {
                case VAR_REGULAR:
                    add_instruction(
                        compiler,
                        (Instruction){
                            .kind = INST_DECLARE_VARIABLE,
                            .declare_variable.ident.kind = IDENT_VAR,
                            .declare_variable.ident.var = s.variable,
                        }
                    );
                    break;
                case VAR_SEMI_SCOPED:
                    break;
                case VAR_ARGUMENT:
                    break;
            }
        }
    }
}

static void
write_section_to_output(CompilerSection* section, FILE* out)
{
    fwrite(section->buffer, section->capacity - section->remaining, 1, out);
}

static void
write_string_constants_table(C_Compiler* compiler, CompilerSection* forward)
{
    write_to_section(forward, DATATYPE_STRING " " STRING_CONSTANTS_TABLE_NAME "[] = {\n");
    for (size_t i = 0; i < compiler->str_hm.count; i++) {
        SourceString str = compiler->str_hm.elements[i];
        if (i > 0) write_to_section(forward, ",\n");
        write_to_section(forward, "{.data=\"");
        write_to_section(forward, str.data);
        write_to_section(forward, "\", .length=");
        char length_as_str[10];
        sprintf(length_as_str, "%zu", str.length);
        write_to_section(forward, length_as_str);
        write_to_section(forward, "}");
    }
    write_to_section(forward, "};\n");
}

typedef enum {
    SEC_FORWARD,
    SEC_TYPEDEFS,
    SEC_DECLARATIONS,
    SEC_DEFS,
    SEC_INIT,
    SEC_MAIN,
    SEC_COUNT
} ProgramSection;

static void
write_instruction_to_section(
    C_Compiler* compiler, CompilerSection* sections, ProgramSection s, Instruction inst
)
{
    switch (inst.kind) {
        case INST_LABEL:
            write_many_to_section(sections + s, inst.label, ":\n", NULL);
            break;
        case INST_GOTO:
            write_many_to_section(sections + s, "goto ", inst.label, ";\n", NULL);
            break;
        case INST_ELSE:
            for (size_t i = 0; i < inst.else_.count; i++)
                write_instruction_to_section(
                    compiler, sections, s, inst.else_.instructions[i]
                );
            break;
        case INST_IF:
            write_many_to_section(
                sections + s,
                "if (",
                (inst.if_.negate) ? "!" : "",
                assignment_to_truthy(compiler, inst.if_.condition_assignment),
                ") {\n",
                NULL
            );
            for (size_t i = 0; i < inst.if_.body.count; i++)
                write_instruction_to_section(
                    compiler, sections, s, inst.if_.body.instructions[i]
                );
            write_to_section(sections + s, "}\n");
            break;
        case INST_LOOP: {
            const char* current = compiler->loop_after;
            compiler->loop_after = inst.loop.after_label;
            // init
            for (size_t i = 0; i < inst.loop.init.count; i++)
                write_instruction_to_section(
                    compiler, sections, s, inst.loop.init.instructions[i]
                );
            // loop
            write_many_to_section(
                sections + s, "while(", inst.loop.condition, ") {\n", NULL
            );
            // before
            for (size_t i = 0; i < inst.loop.before.count; i++)
                write_instruction_to_section(
                    compiler, sections, s, inst.loop.before.instructions[i]
                );
            // body
            for (size_t i = 0; i < inst.loop.body.count; i++)
                write_instruction_to_section(
                    compiler, sections, s, inst.loop.body.instructions[i]
                );
            // after
            for (size_t i = 0; i < inst.loop.after.count; i++)
                write_instruction_to_section(
                    compiler, sections, s, inst.loop.after.instructions[i]
                );
            write_to_section(sections + s, "}\n");
            compiler->loop_after = current;
            break;
        }
        case INST_NO_OP:
            break;
        case INST_ASSIGNMENT:
            write_many_to_section(
                sections + s,
                ident(inst.assignment.left),
                " = ",
                inst.assignment.right,
                ";\n",
                NULL
            );
            break;
        case INST_DECLARE_VARIABLE:
            // TODO: instead of using stringbuilder write directly into section
            write_many_to_section(
                sections +
                    ((s == SEC_INIT && inst.declare_variable.ident.kind == IDENT_VAR)
                         ? SEC_DECLARATIONS
                         : s),
                (inst.declare_variable.ctype)
                    ? inst.declare_variable.ctype
                    : type_info_to_c_syntax(
                          &compiler->sb,
                          (inst.declare_variable.ident.kind == IDENT_VAR)
                              ? inst.declare_variable.ident.var->type_info
                              : inst.declare_variable.type_info
                      ),
                " ",
                ident(inst.declare_variable.ident),
                ";\n",
                NULL
            );
            break;
        case INST_EXPRESSION:
            write_many_to_section(sections + s, inst.expression, ";\n", NULL);
            break;
        case INST_BREAK:
            write_to_section(sections + s, "break;\n");
            break;
        case INST_CONTINUE:
            assert(compiler->loop_after);
            write_many_to_section(
                sections + s, "goto ", compiler->loop_after, ";\n", NULL
            );
            break;
        case INST_DEFINE_CLASS:
            write_to_section(sections + SEC_TYPEDEFS, "typedef struct { ");
            for (size_t i = 0; i < inst.define_class.signature.params_count; i++) {
                write_many_to_section(
                    sections + SEC_TYPEDEFS,
                    type_info_to_c_syntax(
                        &compiler->sb, inst.define_class.signature.types[i]
                    ),
                    " ",
                    inst.define_class.signature.params[i].data,
                    "; ",
                    NULL
                );
            }
            write_many_to_section(
                sections + SEC_TYPEDEFS, "} ", inst.define_class.class_name, ";\n", NULL
            );
            for (size_t i = 0; i < inst.define_class.body.count; i++) {
                Instruction body_inst = inst.define_class.body.instructions[i];
                if (body_inst.kind != INST_DEFINE_FUNCTION)
                    UNIMPLEMENTED(
                        "only function definitions are currently implemented within "
                        "class scope"
                    );
                write_instruction_to_section(compiler, sections, s, body_inst);
            }
            break;
        case INST_DEFINE_FUNCTION: {
            // `return_type function_name(NpContext __ctx__`
            write_many_to_section(
                sections + SEC_DEFS,
                type_info_to_c_syntax(
                    &compiler->sb, inst.define_function.signature.return_type
                ),
                " ",
                inst.define_function.function_name,
                "(NpContext __ctx__",
                NULL
            );

            // `, type1 param1` for each param
            for (size_t i = 0; i < inst.define_function.signature.params_count; i++) {
                write_many_to_section(
                    sections + SEC_DEFS,
                    ", ",
                    type_info_to_c_syntax(
                        &compiler->sb, inst.define_function.signature.types[i]
                    ),
                    " ",
                    inst.define_function.signature.params[i].data,
                    NULL
                );
            }

            // begin scope -- function body -- end scope
            write_to_section(sections + SEC_DEFS, ") {\n");
            for (size_t i = 0; i < inst.define_function.body.count; i++) {
                Instruction body_inst = inst.define_function.body.instructions[i];
                write_instruction_to_section(compiler, sections, SEC_DEFS, body_inst);
            }
            write_to_section(sections + SEC_DEFS, "}\n");

            // declare and assign public NpFunction variable
            write_instruction_to_section(
                compiler,
                sections,
                SEC_DECLARATIONS,
                (Instruction){
                    .kind = INST_DECLARE_VARIABLE,
                    .declare_variable = inst.define_function.public_variable,
                }
            );
            write_instruction_to_section(
                compiler,
                sections,
                SEC_INIT,
                (Instruction){
                    .kind = INST_ASSIGNMENT,
                    .assignment = inst.define_function.assign_fn_variable,
                }
            );
        }
    }
}

static void
write_to_output(C_Compiler* compiler, FILE* out)
{
    CompilerSection sections[SEC_COUNT] = {0};

#if DEBUG
    write_to_section(sections + SEC_FORWARD, "\n// FORWARD COMPILER SECTION\n");
    write_to_section(sections + SEC_TYPEDEFS, "\n// TYPEDEFS COMPILER SECTION\n");
    write_to_section(sections + SEC_DECLARATIONS, "\n// DECLARATIONS COMPILER SECTION\n");
    write_to_section(sections + SEC_DEFS, "\n// FUNCTION DEFINITIONS COMPILER SECTION\n");
    write_to_section(sections + SEC_INIT, "\n// INIT MODULE FUNCTION COMPILER SECTION\n");
    write_to_section(sections + SEC_MAIN, "\n// MAIN FUNCTION COMPILER SECTION\n");
#endif

    write_to_section(sections + SEC_FORWARD, "#include <not_python.h>\n");
    write_to_section(sections + SEC_INIT, "static void init_module(void) {\n");
    write_to_section(sections + SEC_MAIN, "int main(void) {\ninit_module();\n");
    write_string_constants_table(compiler, sections + SEC_FORWARD);

    if (compiler->reqs.libs[LIB_MATH])
        write_to_section(sections + SEC_FORWARD, "#include <math.h>\n");

    // TODO: write instructions
    for (size_t i = 0; i < compiler->instructions.count; i++) {
        Instruction inst = compiler->instructions.instructions[i];
        write_instruction_to_section(compiler, sections, SEC_INIT, inst);
    }

    write_to_section(sections + SEC_INIT, "}");
    write_to_section(sections + SEC_MAIN, "return 0;\n}");

    for (ProgramSection s = 0; s < SEC_COUNT; s++) {
        write_section_to_output(sections + s, out);
        section_free(sections + s);
    }

    fflush(out);
}

Requirements
compile_to_c(FILE* outfile, Lexer* lexer)
{
    TypeChecker tc = {.arena = lexer->arena};
    C_Compiler compiler = {
        .arena = lexer->arena,
        .file_index = lexer->index,
        .top_level_scope = lexer->top_level,
        .instructions = compound_instruction_init(lexer->arena),
        .tc.arena = lexer->arena,
        .sb = sb_init()};
    compiler.tc.stack = &compiler.scope_stack;

    scope_stack_push(&compiler.scope_stack, lexer->top_level);
    declare_scope_variables(&compiler, compiler.top_level_scope);
    for (size_t i = 0; i < lexer->n_statements; i++)
        compile_statement(&compiler, lexer->statements[i]);
    compound_instruction_finalize(&compiler.instructions);

    write_to_output(&compiler, outfile);

    str_hm_free(&compiler.str_hm);
    sb_free(&compiler.sb);

    return compiler.reqs;
}
