#include "compiler.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "diagnostics.h"
#include "lexer_helpers.h"
#include "not_python.h"
#include "np_hash.h"
#include "object_model.h"
#include "syntax.h"
#include "type_checker.h"

#define SEQ_STACK_CAP 128

typedef struct {
    InstructionSequence data[SEQ_STACK_CAP];
    size_t count;
} SequenceStack;

static void seq_stack_push(SequenceStack* stack, InstructionSequence seq);
static void seq_stack_push_new(SequenceStack* stack, Arena* arena);
static InstructionSequence seq_stack_pop(SequenceStack* stack);
static void seq_stack_append_instruction(SequenceStack* stack, Instruction inst);

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
    StorageIdent none_ident;
    StorageIdent empty_str_ident;
    size_t unique_vars_counter;
    const char* excepts_goto;
    const char* loop_after;
    SequenceStack inst_seq_stack;
} Compiler;

#define UNIQUE_ID(compiler)                                                              \
    arena_snprintf(compiler->arena, 16, "_np_%zu", compiler->unique_vars_counter++)

static size_t str_hm_put(StringHashmap* hm, SourceString element);
static void str_hm_free(StringHashmap* hm);

static void compile_statement(Compiler* compiler, Statement* stmt);
static void compile_set_item(
    Compiler* compiler, StorageIdent container, StorageIdent key, StorageIdent value
);
static void declare_scope_variables(Compiler* compiler, LexicalScope* scope);

// For hinting to a function where the output should be stored
typedef StorageIdent StorageHint;

// A hint that will require an identifier to be generated
#define IS_NULL_IDENTIFIER(hint)                                                         \
    (hint.kind > IDENT_VAR || (hint.kind == IDENT_CSTR && !hint.cstr))

// No information or requirements for storage present
#define NULL_HINT                                                                        \
    (StorageHint) { 0 }

static StorageIdent render_expression(
    Compiler* compiler, StorageHint hint, Expression* expr
);

static StorageIdent convert_to_string(Compiler* compiler, StorageIdent id);
static StorageIdent convert_to_truthy(Compiler* compiler, StorageIdent id);
static StorageIdent storage_ident_from_hint(Compiler* compiler, StorageHint hint);
static StorageIdent render_object_method_call(
    Compiler* compiler,
    StorageHint hint,
    StorageIdent self_ident,
    FunctionStatement* fndef,
    Arguments* args
);

static InstructionSequence instruction_sequence_init(Arena* arena);
static void instruction_sequence_finalize(InstructionSequence* sequence);
static void add_instruction(Compiler* compiler, Instruction inst);

#define SOURCESTRING(string)                                                             \
    (SourceString) { .data = string, .length = sizeof(string) - 1 }

CompiledInstructions
compile(Lexer* lexer)
{
    Compiler compiler = {
        .arena = lexer->arena,
        .file_index = lexer->index,
        .top_level_scope = lexer->top_level,
        .tc.arena = lexer->arena,
    };
    seq_stack_push_new(&compiler.inst_seq_stack, compiler.arena);
    compiler.none_ident = (StorageIdent){
        .kind = IDENT_INT_LITERAL,
        .int_value = 0,
        .info = NONE_TYPE,
    };
    compiler.empty_str_ident = (StorageIdent){
        .kind = IDENT_STRING_LITERAL,
        .str_literal_index = str_hm_put(&compiler.str_hm, SOURCESTRING(""))};
    compiler.tc.stack = &compiler.scope_stack;

    scope_stack_push(&compiler.scope_stack, lexer->top_level);
    declare_scope_variables(&compiler, compiler.top_level_scope);
    for (size_t i = 0; i < lexer->n_statements; i++)
        compile_statement(&compiler, lexer->statements[i]);

    assert(compiler.inst_seq_stack.count == 1);

    return (CompiledInstructions){
        .seq = seq_stack_pop(&compiler.inst_seq_stack),
        .req = compiler.reqs,
        // TODO: make sure writer frees this when no longer needed
        .str_constants = compiler.str_hm};
}

static InstructionSequence
instruction_sequence_init(Arena* arena)
{
    InstructionSequence seq = {
        .arena = arena,
        .capacity = 8,
        .instructions = arena_dynamic_alloc(arena, sizeof(Instruction) * 8),
    };
    return seq;
}

void
instruction_sequence_append(InstructionSequence* sequence, Instruction inst)
{
    if (sequence->count == sequence->capacity) {
        sequence->capacity *= 2;
        sequence->instructions = arena_dynamic_realloc(
            sequence->arena,
            sequence->instructions,
            sizeof(Instruction) * sequence->capacity
        );
    }
    sequence->instructions[sequence->count++] = inst;
}

void
instruction_sequence_finalize(InstructionSequence* sequence)
{
    sequence->instructions =
        arena_dynamic_finalize(sequence->arena, sequence->instructions);
}

static void
seq_stack_push(SequenceStack* stack, InstructionSequence seq)
{
    if (stack->count == SEQ_STACK_CAP) errorf("sequence stack overflow");
    stack->data[stack->count++] = seq;
}

static void
seq_stack_push_new(SequenceStack* stack, Arena* arena)
{
    seq_stack_push(stack, instruction_sequence_init(arena));
}

static InstructionSequence
seq_stack_pop(SequenceStack* stack)
{
    if (stack->count == 0) errorf("popping from empty SequenceStack");
    InstructionSequence* seq = stack->data + ((stack->count--) - 1);
    instruction_sequence_finalize(seq);
    return *seq;
}

static void
seq_stack_append_instruction(SequenceStack* stack, Instruction inst)
{
    if (stack->count == 0) error("appending instruction to empty sequence stack");
    instruction_sequence_append(stack->data + stack->count - 1, inst);
}

static void
add_instruction(Compiler* compiler, Instruction inst)
{
    if (inst.kind == INST_DECLARE_VARIABLE && inst.declare_variable.kind == IDENT_CSTR &&
        inst.declare_variable.info.type == NPTYPE_UNTYPED) {
        assert(0 && "trying to add untyped variable declaration");
    }
    seq_stack_append_instruction(&compiler->inst_seq_stack, inst);
}

#define COMPILER_ACCUMULATE_INSTRUCTIONS(compiler, dest)                                 \
    for (int MACRO_VAR(i) =                                                              \
             (seq_stack_push_new(&compiler->inst_seq_stack, compiler->arena), 0);        \
         MACRO_VAR(i) == 0;                                                              \
         dest = seq_stack_pop(&compiler->inst_seq_stack), MACRO_VAR(i) = 1)

static StorageIdent
storage_ident_from_variable(Variable* variable)
{
    switch (variable->kind) {
        case VAR_REGULAR:
            return (StorageIdent){
                .kind = IDENT_VAR, .var = variable, .info = variable->type_info};
        case VAR_CLOSURE:
            return (StorageIdent){
                .kind = IDENT_VAR, .var = variable, .info = variable->type_info};
        case VAR_SEMI_SCOPED:
            return (StorageIdent){
                .kind = IDENT_CSTR,
                .cstr = variable->compiled_name.data,
                .info = variable->type_info};
        case VAR_ARGUMENT:
            return (StorageIdent){
                .kind = IDENT_VAR, .var = variable, .info = variable->type_info};
    }
    UNREACHABLE();
}

static StorageIdent
storage_ident_from_hint(Compiler* compiler, StorageHint hint)
{
    if (IS_NULL_IDENTIFIER(hint)) {
        TypeInfo info;
        switch (hint.kind) {
            case IDENT_FLOAT_LITERAL:
                info = FLOAT_TYPE;
                break;
            case IDENT_INT_LITERAL:
                info = INT_TYPE;
                break;
            case IDENT_STRING_LITERAL:
                info = STRING_TYPE;
                break;
            default:
                info = hint.info;
                break;
        }
        return (StorageIdent){
            .kind = IDENT_CSTR,
            .cstr = UNIQUE_ID(compiler),
            .info = info,
        };
    }
    return hint;
}

static StorageIdent
storage_ident_from_fndef(FunctionStatement* fndef)
{
    return (StorageIdent){
        .kind = IDENT_CSTR,
        .cstr = fndef->ns_ident.data,
        .info = (TypeInfo){.type = NPTYPE_FUNCTION, .sig = &fndef->sig},
    };
}

static void
set_storage_type_info(Compiler* compiler, StorageIdent* ident, TypeInfo info)
{
    assert(info.type != NPTYPE_UNTYPED && "trying to set type info to untyped");

    if (ident->info.type != NPTYPE_UNTYPED)
        error("trying to set the type on an AssignmentInst multiple times");

    ident->info = info;

    switch (ident->kind) {
        case IDENT_CSTR:
            break;
        case IDENT_INT_LITERAL:
            break;
        case IDENT_FLOAT_LITERAL:
            break;
        case IDENT_STRING_LITERAL:
            break;
        case IDENT_VAR: {
            ident->info = info;
            Variable* var = ident->var;
            if (ident->var->type_info.type == NPTYPE_UNTYPED) {
                var->type_info = info;
            }
            else if (!compare_types(var->type_info, info)) {
                // TODO: location might not be accurate here
                type_errorf(
                    compiler->file_index,
                    compiler->current_operation_location,
                    "variable `%s` of previously defined type `%s` trying to have type "
                    "`%s` assigned",
                    var->identifier.data,
                    errfmt_type_info(var->type_info),
                    errfmt_type_info(info)
                );
            }
        } break;
    }
}

static void
check_storage_type_info(Compiler* compiler, StorageIdent* ident, TypeInfo type_info)
{
    assert(type_info.type != NPTYPE_UNTYPED && "trying to set type info to untyped");

    if (ident->info.type == NPTYPE_UNTYPED)
        set_storage_type_info(compiler, ident, type_info);
    else if (ident->kind == IDENT_VAR && ident->var->type_info.type == NPTYPE_UNTYPED)
        ident->var->type_info = type_info;
    else if (!compare_types(ident->info, type_info))
        // TODO: location might not be accurate here
        type_errorf(
            compiler->file_index,
            compiler->current_operation_location,
            "expecting type `%s` but got type `%s`",
            errfmt_type_info(ident->info),
            errfmt_type_info(type_info)
        );
}

SourceString
create_default_object_fmt_str(Arena* arena, ClassStatement* clsdef)
{
    // if the fmt string overflows 1024 it will just be cut short.
    char fmtbuf[1024];
    size_t remaining = 1023;
    char* write = fmtbuf;
    if (remaining >= clsdef->name.length) {
        memcpy(write, clsdef->name.data, clsdef->name.length);
        write += clsdef->name.length;
        remaining -= clsdef->name.length;
    }
    if (remaining) {
        *write++ = '(';
        remaining--;
    }
    for (size_t i = 0; i < clsdef->sig.params_count; i++) {
        if (remaining > 1 && i > 0) {
            *write++ = ',';
            *write++ = ' ';
            remaining -= 2;
        }
        SourceString param = clsdef->sig.params[i];
        if (remaining >= param.length) {
            memcpy(write, param.data, param.length);
            write += param.length;
            remaining -= param.length;
        }
        if (remaining > 2) {
            *write++ = '=';
            *write++ = '%';
            *write++ = 's';
            remaining -= 3;
        }
    }
    if (remaining) {
        *write++ = ')';
        remaining--;
    }

    SourceString rtval = {
        .data = arena_alloc(arena, 1024 - remaining), .length = 1023 - remaining};
    memcpy((char*)rtval.data, fmtbuf, rtval.length);
    return rtval;
}

static const char* SORT_CMP_TABLE[NPTYPE_COUNT] = {
    [NPTYPE_INT] = "np_int_sort_fn",
    [NPTYPE_FLOAT] = "np_float_sort_fn",
    [NPTYPE_BOOL] = "np_bool_sort_fn",
    [NPTYPE_STRING] = "np_str_sort_fn",
};

static const char* REVERSE_SORT_CMP_TABLE[NPTYPE_COUNT] = {
    [NPTYPE_INT] = "np_int_sort_fn_rev",
    [NPTYPE_FLOAT] = "np_float_sort_fn_rev",
    [NPTYPE_BOOL] = "np_bool_sort_fn_rev",
    [NPTYPE_STRING] = "np_str_sort_fn_rev",
};

const char*
sort_cmp_for_type_info(TypeInfo type_info, bool reversed)
{
    const char* rtval;
    if (reversed)
        rtval = REVERSE_SORT_CMP_TABLE[type_info.type];
    else
        rtval = SORT_CMP_TABLE[type_info.type];
    if (!rtval) return "NULL";
    return rtval;
}

static const char* VOIDPTR_CMP_TABLE[NPTYPE_COUNT] = {
    [NPTYPE_INT] = "np_void_int_eq",
    [NPTYPE_FLOAT] = "np_void_float_eq",
    [NPTYPE_BOOL] = "np_void_bool_eq",
    [NPTYPE_STRING] = "np_void_str_eq",
};

static const char* CMP_TABLE[NPTYPE_COUNT] = {
    [NPTYPE_INT] = "np_int_eq",
    [NPTYPE_FLOAT] = "np_float_eq",
    [NPTYPE_BOOL] = "np_bool_eq",
    [NPTYPE_STRING] = "np_str_eq",
};

const char*
voidptr_cmp_for_type_info(TypeInfo type_info)
{
    const char* cmp_for_type = VOIDPTR_CMP_TABLE[type_info.type];
    if (!cmp_for_type) return "NULL";
    return cmp_for_type;
}

const char*
cmp_for_type_info(TypeInfo type_info)
{
    const char* cmp_for_type = CMP_TABLE[type_info.type];
    if (!cmp_for_type) return "NULL";
    return cmp_for_type;
}

uint64_t
source_string_to_exception_type_flag(FileIndex index, Location loc, SourceString str)
{
    // TODO: the parser should probably do this, wait to change until
    // exception handling has been refactored to callback system.
    static const SourceString MemoryError = {.data = "MemoryError", .length = 11};
    static const SourceString IndexError = {.data = "IndexError", .length = 10};
    static const SourceString KeyError = {.data = "KeyError", .length = 8};
    static const SourceString ValueError = {.data = "ValueError", .length = 10};
    static const SourceString AssertionError = {.data = "AssertionError", .length = 14};

    if (SOURCESTRING_EQ(str, MemoryError)) return MEMORY_ERROR;
    if (SOURCESTRING_EQ(str, IndexError)) return INDEX_ERROR;
    if (SOURCESTRING_EQ(str, ValueError)) return VALUE_ERROR;
    if (SOURCESTRING_EQ(str, KeyError)) return KEY_ERROR;
    if (SOURCESTRING_EQ(str, AssertionError)) return ASSERTION_ERROR;
    unspecified_errorf(index, loc, "unrecognized exception type `%s`", str.data);
    UNREACHABLE();
}

static void
bind_self_to_function_object(Compiler* compiler, StorageIdent self, StorageIdent func)
{
    StorageIdent ctx_object =
        storage_ident_from_hint(compiler, (StorageHint){.info = CONTEXT_TYPE});
    add_instruction(
        compiler,
        (Instruction){
            .kind = INST_DECL_ASSIGNMENT,
            .assignment.left = ctx_object,
            .assignment.right =
                (OperationInst){
                    .kind = OPERATION_GET_ATTR,
                    .object = func,
                    .attr = SOURCESTRING("ctx"),
                }}
    );
    add_instruction(
        compiler,
        (Instruction){
            .kind = INST_OPERATION,
            .operation =
                (OperationInst){
                    .kind = OPERATION_SET_ATTR,
                    .object = ctx_object,
                    .attr = SOURCESTRING("self"),
                    .value = self,
                }}
    );
    add_instruction(
        compiler,
        (Instruction){
            .kind = INST_OPERATION,
            .operation =
                (OperationInst){
                    .kind = OPERATION_SET_ATTR,
                    .object = func,
                    .attr = SOURCESTRING("ctx"),
                    .value = ctx_object,
                }}
    );
}

static Symbol*
get_symbol(Compiler* compiler, SourceString identifier)
{
    // TODO: need to treat globals/locals differently in the furute
    return get_symbol_from_scopes(compiler->scope_stack, identifier);
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

static StorageIdent
render_function_object_copy(
    Compiler* compiler, StorageHint hint, FunctionStatement* fndef
)
{
    StorageIdent rtval = storage_ident_from_hint(compiler, hint);
    check_storage_type_info(
        compiler, &rtval, (TypeInfo){.type = NPTYPE_FUNCTION, .sig = &fndef->sig}
    );

    add_instruction(
        compiler,
        (Instruction){
            .kind = (IS_NULL_IDENTIFIER(hint)) ? INST_DECL_ASSIGNMENT : INST_ASSIGNMENT,
            .assignment.left = rtval,
            .assignment.right =
                (OperationInst){
                    .kind = OPERATION_COPY,
                    .copy = storage_ident_from_fndef(fndef),
                },
        }
    );

    return rtval;
}

static StorageIdent
render_default_object_string_representation(
    Compiler* compiler, StorageHint hint, StorageIdent object_ident
)
{
    ClassStatement* clsdef = object_ident.info.cls;
    //  ClassName(param1=param1_value, param2=param2_value...)
    if (!clsdef->fmtstr.data) {
        // default fmt string expects all params to be first convereted into cstr
        clsdef->fmtstr = create_default_object_fmt_str(compiler->arena, clsdef);
        clsdef->fmtstr_index = str_hm_put(&compiler->str_hm, clsdef->fmtstr);
    }

    // room for each member twice plus 1 for the fmt string
    // (each member needs an additional ident slot for the NPLIB_STR_TO_CSTR call later)
    size_t argc = clsdef->sig.params_count + 1;
    StorageIdent* arg_idents =
        arena_alloc(compiler->arena, sizeof(StorageIdent) * argc * 2);

    // put fmtstring into a cstr typed variable
    StorageIdent fmtstr_ident =
        storage_ident_from_hint(compiler, (StorageHint){.info.type = NPTYPE_CSTR});
    arg_idents[0] = fmtstr_ident;

    add_instruction(
        compiler,
        (Instruction){
            .kind = INST_DECL_ASSIGNMENT,
            .assignment.left = fmtstr_ident,
            .assignment.right =
                (OperationInst){
                    .kind = OPERATION_GET_ATTR,
                    .object =
                        (StorageIdent){
                            .kind = IDENT_STRING_LITERAL,
                            .str_literal_index = clsdef->fmtstr_index,
                            .info = STRING_TYPE,
                        },
                    .attr = SOURCESTRING("data"),
                },
        }
    );

    // convert each member into a cstr
    for (size_t i = 1; i < argc; i++) {
        StorageIdent attr_ident = storage_ident_from_hint(
            compiler, (StorageHint){.info = object_ident.info.cls->sig.types[i - 1]}
        );
        add_instruction(
            compiler,
            (Instruction){
                .kind = INST_DECL_ASSIGNMENT,
                .assignment.left = attr_ident,
                .assignment.right =
                    (OperationInst){
                        .kind = OPERATION_GET_ATTR,
                        .object = object_ident,
                        .attr = object_ident.info.cls->sig.params[i - 1],
                    },
            }
        );

        arg_idents[argc + i - 1] = convert_to_string(compiler, attr_ident);

        StorageIdent attr_as_cstr_ident =
            storage_ident_from_hint(compiler, (StorageHint){.info.type = NPTYPE_CSTR});
        add_instruction(
            compiler,
            (Instruction){
                .kind = INST_DECL_ASSIGNMENT,
                .assignment.left = attr_as_cstr_ident,
                .assignment.right =
                    (OperationInst){
                        .kind = OPERATION_C_CALL,
                        .c_fn_name = NPLIB_STR_TO_CSTR,
                        .c_fn_argc = 1,
                        .c_fn_argv = arg_idents + argc + i - 1,
                    },
            }
        );
        arg_idents[i] = attr_as_cstr_ident;
    }

    StorageIdent result_ident = storage_ident_from_hint(compiler, hint);
    check_storage_type_info(compiler, &result_ident, STRING_TYPE);
    add_instruction(
        compiler,
        (Instruction){
            .kind = (IS_NULL_IDENTIFIER(hint)) ? INST_DECL_ASSIGNMENT : INST_ASSIGNMENT,
            .assignment.left = result_ident,
            .assignment.right =
                (OperationInst){
                    .kind = OPERATION_C_CALL,
                    .c_fn_name = NPLIB_STR_FMT,
                    .c_fn_argc = argc,
                    .c_fn_argv = arg_idents,
                },
        }
    );

    return result_ident;
}

static StorageIdent
convert_to_string(Compiler* compiler, StorageIdent id)
{
    OperationInst operation;

    switch (id.info.type) {
        case NPTYPE_INT:
            operation = (OperationInst){
                .kind = OPERATION_C_CALL1,
                .c_fn_name = NPLIB_CONVERSION_FUNCTIONS[NPTYPE_INT][NPTYPE_STRING],
                .c_fn_argc = 1,
                .c_fn_arg = id};
            break;
        case NPTYPE_FLOAT:
            operation = (OperationInst){
                .kind = OPERATION_C_CALL1,
                .c_fn_name = NPLIB_CONVERSION_FUNCTIONS[NPTYPE_FLOAT][NPTYPE_STRING],
                .c_fn_argc = 1,
                .c_fn_arg = id};
            break;
        case NPTYPE_BOOL:
            operation = (OperationInst){
                .kind = OPERATION_C_CALL1,
                .c_fn_name = NPLIB_CONVERSION_FUNCTIONS[NPTYPE_BOOL][NPTYPE_STRING],
                .c_fn_argc = 1,
                .c_fn_arg = id};
            break;
        case NPTYPE_OBJECT: {
            FunctionStatement* user_defined_str =
                id.info.cls->object_model_methods[OBJECT_MODEL_STR];
            if (user_defined_str) {
                Arguments args = {0};
                return render_object_method_call(
                    compiler, NULL_HINT, id, user_defined_str, &args
                );
            }
            else
                return render_default_object_string_representation(
                    compiler, NULL_HINT, id
                );
            break;
        }
        default: {
            unspecified_errorf(
                compiler->file_index,
                compiler->current_stmt_location,
                "type conversion from `%s` to `str` not currently implemented",
                errfmt_type_info(id.info)
            );
        }
    }
    StorageIdent result_id =
        storage_ident_from_hint(compiler, (StorageHint){.info = STRING_TYPE});
    add_instruction(
        compiler,
        (Instruction){
            .kind = INST_DECL_ASSIGNMENT,
            .assignment.left = result_id,
            .assignment.right = operation,
        }
    );
    return result_id;
}

static StorageIdent
render_builtin_print(Compiler* compiler, StorageHint hint, Arguments* args)
{
    if (args->values_count != args->n_positional) {
        type_error(
            compiler->file_index,
            compiler->current_operation_location,
            "print keyword arguments not currently implemented"
        );
    }

    StorageIdent rtval = storage_ident_from_hint(compiler, hint);
    check_storage_type_info(compiler, &rtval, NONE_TYPE);

    size_t args_count = (args->values_count == 0) ? 2 : args->values_count + 1;
    StorageIdent* argv = arena_alloc(compiler->arena, sizeof(StorageIdent) * args_count);
    argv[0] = (StorageIdent){.kind = IDENT_INT_LITERAL, .int_value = args_count - 1};
    if (args->values_count == 0)
        argv[1] = compiler->empty_str_ident;
    else {
        for (size_t i = 0; i < args->values_count; i++) {
            StorageIdent arg_id = render_expression(compiler, NULL_HINT, args->values[i]);
            if (arg_id.info.type != NPTYPE_STRING) {
                arg_id = convert_to_string(compiler, arg_id);
            }
            argv[1 + i] = arg_id;
        }
    }

    add_instruction(
        compiler,
        (Instruction){
            .kind = (IS_NULL_IDENTIFIER(hint)) ? INST_DECL_ASSIGNMENT : INST_ASSIGNMENT,
            .assignment.left = rtval,
            .assignment.right =
                (OperationInst){
                    .kind = OPERATION_C_CALL,
                    .c_fn_name = NPLIB_PRINT,
                    .c_fn_argc = args_count,
                    .c_fn_argv = argv,
                },
        }
    );

    return rtval;
}

static void
expect_arg_count(Compiler* compiler, char* fn_name, Arguments* args, size_t count)
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

static StorageIdent
render_list_builtin(
    Compiler* compiler,
    StorageHint hint,
    StorageIdent list_ident,
    const char* fn_name,
    Arguments* args
)
{
    // TODO: stop using out variable in library functions

    // TODO: the function objects could probably now handle making references
    // to these calls and calling them through the standard mechanisms while
    // this logic could shift into creating the function objects

    assert(fn_name && "fn_name cannot be NULL");
    TypeInfo list_content_type = list_ident.info.inner->types[0];

    bool req_decl = IS_NULL_IDENTIFIER(hint);
    StorageIdent rtval = storage_ident_from_hint(compiler, hint);

    switch (fn_name[0]) {
        case 'a':
            if (strcmp(fn_name, "append") == 0) {
                expect_arg_count(compiler, "list.append", args, 1);
                check_storage_type_info(compiler, &rtval, NONE_TYPE);

                StorageIdent* argv =
                    arena_alloc(compiler->arena, sizeof(StorageIdent) * 2);
                argv[0] = list_ident;
                argv[1] = render_expression(compiler, NULL_HINT, args->values[0]);
                argv[1].reference = true;

                add_instruction(
                    compiler,
                    (Instruction){
                        .kind = (req_decl) ? INST_DECL_ASSIGNMENT : INST_ASSIGNMENT,
                        .assignment.left = rtval,
                        .assignment.right =
                            (OperationInst){
                                .kind = OPERATION_C_CALL,
                                .c_fn_name = NPLIB_LIST_APPEND,
                                .c_fn_argc = 2,
                                .c_fn_argv = argv,
                            },
                    }
                );
                return rtval;
            }
            break;
        case 'c':
            if (strcmp(fn_name, "clear") == 0) {
                expect_arg_count(compiler, "list.clear", args, 0);
                check_storage_type_info(compiler, &rtval, NONE_TYPE);

                add_instruction(
                    compiler,
                    (Instruction){
                        .kind = (req_decl) ? INST_DECL_ASSIGNMENT : INST_ASSIGNMENT,
                        .assignment.left = rtval,
                        .assignment.right =
                            (OperationInst){
                                .kind = OPERATION_C_CALL1,
                                .c_fn_name = NPLIB_LIST_CLEAR,
                                .c_fn_argc = 1,
                                .c_fn_arg = list_ident,
                            },
                    }
                );
                return rtval;
            }
            else if (strcmp(fn_name, "copy") == 0) {
                expect_arg_count(compiler, "list.copy", args, 0);
                check_storage_type_info(compiler, &rtval, list_ident.info);

                add_instruction(
                    compiler,
                    (Instruction){
                        .kind = (req_decl) ? INST_DECL_ASSIGNMENT : INST_ASSIGNMENT,
                        .assignment.left = rtval,
                        .assignment.right =
                            (OperationInst){
                                .kind = OPERATION_C_CALL1,
                                .c_fn_name = NPLIB_LIST_COPY,
                                .c_fn_argc = 1,
                                .c_fn_arg = list_ident,
                            },
                    }
                );
                return rtval;
            }
            else if (strcmp(fn_name, "count") == 0) {
                expect_arg_count(compiler, "list.count", args, 1);
                check_storage_type_info(compiler, &rtval, INT_TYPE);

                StorageIdent* argv =
                    arena_alloc(compiler->arena, sizeof(StorageIdent) * 2);
                argv[0] = list_ident;
                argv[1] = render_expression(
                    compiler, (StorageHint){.info = list_content_type}, args->values[0]
                );
                argv[1].reference = true;

                add_instruction(
                    compiler,
                    (Instruction){
                        .kind = (req_decl) ? INST_DECL_ASSIGNMENT : INST_ASSIGNMENT,
                        .assignment.left = rtval,
                        .assignment.right =
                            (OperationInst){
                                .kind = OPERATION_C_CALL,
                                .c_fn_name = NPLIB_LIST_COUNT,
                                .c_fn_argc = 2,
                                .c_fn_argv = argv,
                            },
                    }
                );
                return rtval;
            }
            break;
        case 'e':
            if (strcmp(fn_name, "extend") == 0) {
                expect_arg_count(compiler, "list.extend", args, 1);
                check_storage_type_info(compiler, &rtval, NONE_TYPE);

                StorageIdent* argv =
                    arena_alloc(compiler->arena, sizeof(StorageIdent) * 2);
                argv[0] = list_ident;
                argv[1] = render_expression(
                    compiler, (StorageHint){.info = list_ident.info}, args->values[0]
                );

                add_instruction(
                    compiler,
                    (Instruction){
                        .kind = (req_decl) ? INST_DECL_ASSIGNMENT : INST_ASSIGNMENT,
                        .assignment.left = rtval,
                        .assignment.right =
                            (OperationInst){
                                .kind = OPERATION_C_CALL,
                                .c_fn_name = NPLIB_LIST_EXTEND,
                                .c_fn_argc = 2,
                                .c_fn_argv = argv,
                            },
                    }
                );
                return rtval;
            }
            break;
        case 'i':
            if (strcmp(fn_name, "index") == 0) {
                // TODO: start/stop
                expect_arg_count(compiler, "list.index", args, 1);
                check_storage_type_info(compiler, &rtval, INT_TYPE);

                StorageIdent* argv =
                    arena_alloc(compiler->arena, sizeof(StorageIdent) * 2);
                argv[0] = list_ident;
                argv[1] = render_expression(
                    compiler, (StorageHint){.info = list_content_type}, args->values[0]
                );
                argv[1].reference = true;

                add_instruction(
                    compiler,
                    (Instruction){
                        .kind = (req_decl) ? INST_DECL_ASSIGNMENT : INST_ASSIGNMENT,
                        .assignment.left = rtval,
                        .assignment.right =
                            (OperationInst){
                                .kind = OPERATION_C_CALL,
                                .c_fn_name = NPLIB_LIST_INDEX,
                                .c_fn_argc = 2,
                                .c_fn_argv = argv,
                            },
                    }
                );
                return rtval;
            }
            else if (strcmp(fn_name, "insert") == 0) {
                expect_arg_count(compiler, "list.insert", args, 2);
                check_storage_type_info(compiler, &rtval, NONE_TYPE);

                StorageIdent* argv =
                    arena_alloc(compiler->arena, sizeof(StorageIdent) * 3);
                argv[0] = list_ident;
                argv[1] = render_expression(
                    compiler, (StorageHint){.info = INT_TYPE}, args->values[0]
                );
                argv[2] = render_expression(
                    compiler, (StorageHint){.info = list_content_type}, args->values[1]
                );
                argv[2].reference = true;

                add_instruction(
                    compiler,
                    (Instruction){
                        .kind = (req_decl) ? INST_DECL_ASSIGNMENT : INST_ASSIGNMENT,
                        .assignment.left = rtval,
                        .assignment.right =
                            (OperationInst){
                                .kind = OPERATION_C_CALL,
                                .c_fn_name = NPLIB_LIST_INSERT,
                                .c_fn_argc = 3,
                                .c_fn_argv = argv,
                            },
                    }
                );
                return compiler->none_ident;
            }
            break;
        case 'p':
            if (strcmp(fn_name, "pop") == 0) {
                StorageIdent* argv =
                    arena_alloc(compiler->arena, sizeof(StorageIdent) * 3);
                check_storage_type_info(compiler, &rtval, list_content_type);

                argv[0] = list_ident;
                if (args->values_count == 0)
                    argv[1] = (StorageIdent){.kind = IDENT_INT_LITERAL, .int_value = -1};
                else if (args->values_count == 1)
                    argv[1] = render_expression(
                        compiler, (StorageHint){.info = INT_TYPE}, args->values[0]
                    );
                else
                    type_errorf(
                        compiler->file_index,
                        compiler->current_operation_location,
                        "list.pop expecting 0-1 arguments but got %zu",
                        args->values_count
                    );

                argv[2] = rtval;
                add_instruction(
                    compiler,
                    (Instruction){
                        .kind = INST_DECLARE_VARIABLE,
                        .declare_variable = argv[2],
                    }
                );
                argv[2].reference = true;

                if (req_decl)
                    add_instruction(
                        compiler,
                        (Instruction){
                            .kind = INST_DECLARE_VARIABLE,
                            .declare_variable = rtval,
                        }
                    );

                add_instruction(
                    compiler,
                    (Instruction){
                        .kind = INST_OPERATION,
                        .operation =
                            (OperationInst){
                                .kind = OPERATION_C_CALL,
                                .c_fn_name = NPLIB_LIST_POP,
                                .c_fn_argc = 3,
                                .c_fn_argv = argv,
                            },
                    }
                );
                return rtval;
            }
            break;
        case 'r':
            if (strcmp(fn_name, "remove") == 0) {
                expect_arg_count(compiler, "list.remove", args, 1);
                check_storage_type_info(compiler, &rtval, NONE_TYPE);

                StorageIdent* argv =
                    arena_alloc(compiler->arena, sizeof(StorageIdent) * 2);

                argv[0] = list_ident;
                argv[1] = render_expression(
                    compiler, (StorageHint){.info = list_content_type}, args->values[0]
                );
                argv[1].reference = true;

                add_instruction(
                    compiler,
                    (Instruction){
                        .kind = (req_decl) ? INST_DECL_ASSIGNMENT : INST_ASSIGNMENT,
                        .assignment.left = rtval,
                        .assignment.right =
                            (OperationInst){
                                .kind = OPERATION_C_CALL,
                                .c_fn_name = NPLIB_LIST_REMOVE,
                                .c_fn_argc = 2,
                                .c_fn_argv = argv,
                            },
                    }
                );
                return rtval;
            }
            else if (strcmp(fn_name, "reverse") == 0) {
                expect_arg_count(compiler, "list.reverse", args, 0);
                check_storage_type_info(compiler, &rtval, NONE_TYPE);

                add_instruction(
                    compiler,
                    (Instruction){
                        .kind = (req_decl) ? INST_DECL_ASSIGNMENT : INST_ASSIGNMENT,
                        .assignment.left = rtval,
                        .assignment.right =
                            (OperationInst){
                                .kind = OPERATION_C_CALL1,
                                .c_fn_name = NPLIB_LIST_REVERSE,
                                .c_fn_argc = 1,
                                .c_fn_arg = list_ident,
                            },
                    }
                );
                return rtval;
            }
            break;
        case 's':
            if (strcmp(fn_name, "sort") == 0) {
                // TODO: accept key function argument
                // validate args
                check_storage_type_info(compiler, &rtval, NONE_TYPE);

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
                        "ERROR: list.sort expecting either 0 args or boolean "
                        "keyword "
                        "arg "
                        "`reverse`\n"
                    );
                    exit(1);
                }

                StorageIdent* argv =
                    arena_alloc(compiler->arena, sizeof(StorageIdent) * 2);

                argv[0] = list_ident;

                // resolve `reverse` argument
                if (args->values_count > 0) {
                    StorageIdent reverse_value =
                        render_expression(compiler, NULL_HINT, args->values[0]);
                    argv[1] = convert_to_truthy(compiler, reverse_value);
                }
                else {
                    argv[1] = (StorageIdent){.kind = IDENT_INT_LITERAL, .int_value = 0};
                }

                // call
                add_instruction(
                    compiler,
                    (Instruction){
                        .kind = (req_decl) ? INST_DECL_ASSIGNMENT : INST_ASSIGNMENT,
                        .assignment.left = rtval,
                        .assignment.right =
                            (OperationInst){
                                .kind = OPERATION_C_CALL,
                                .c_fn_name = NPLIB_LIST_SORT,
                                .c_fn_argc = 2,
                                .c_fn_argv = argv,
                            },
                    }
                );
                return rtval;
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
    UNREACHABLE();
}

static StorageIdent
render_dict_builtin(
    Compiler* compiler,
    StorageHint hint,
    StorageIdent dict_ident,
    const char* fn_name,
    Arguments* args
)
{
    assert(fn_name && "fn_name cannot be NULL");

    TypeInfo key_type = dict_ident.info.inner->types[0];
    TypeInfo val_type = dict_ident.info.inner->types[1];
    StorageIdent rtval = storage_ident_from_hint(compiler, hint);
    bool req_decl = IS_NULL_IDENTIFIER(hint);

    switch (fn_name[0]) {
        case 'c':
            if (strcmp(fn_name, "clear") == 0) {
                expect_arg_count(compiler, "dict.clear", args, 0);
                check_storage_type_info(compiler, &rtval, NONE_TYPE);

                add_instruction(
                    compiler,
                    (Instruction){
                        .kind = (req_decl) ? INST_DECL_ASSIGNMENT : INST_ASSIGNMENT,
                        .assignment.left = rtval,
                        .assignment.right =
                            (OperationInst){
                                .kind = OPERATION_C_CALL1,
                                .c_fn_name = NPLIB_DICT_CLEAR,
                                .c_fn_argc = 1,
                                .c_fn_arg = dict_ident,
                            },
                    }
                );
                return rtval;
            }
            else if (strcmp(fn_name, "copy") == 0) {
                expect_arg_count(compiler, "dict.copy", args, 0);
                check_storage_type_info(compiler, &rtval, dict_ident.info);

                add_instruction(
                    compiler,
                    (Instruction){
                        .kind = (req_decl) ? INST_DECL_ASSIGNMENT : INST_ASSIGNMENT,
                        .assignment.left = rtval,
                        .assignment.right =
                            (OperationInst){
                                .kind = OPERATION_C_CALL1,
                                .c_fn_name = NPLIB_DICT_COPY,
                                .c_fn_argc = 1,
                                .c_fn_arg = dict_ident,
                            },
                    }
                );
                return rtval;
            }
            break;
        case 'g':
            if (strcmp(fn_name, "get") == 0) {
                UNIMPLEMENTED("dict.get is not implemented");
                return compiler->none_ident;
            }
            break;
        case 'i':
            if (strcmp(fn_name, "items") == 0) {
                expect_arg_count(compiler, "dict.items", args, 0);

                // Iter[DictItems[KeyType, ValueType]]
                TypeInfo dict_items_type = {
                    .type = NPTYPE_DICT_ITEMS, .inner = dict_ident.info.inner};
                TypeInfo return_type = {
                    .type = NPTYPE_ITER,
                    .inner = arena_alloc(compiler->arena, sizeof(TypeInfoInner))};
                return_type.inner->types = arena_alloc(compiler->arena, sizeof(TypeInfo));
                return_type.inner->types[0] = dict_items_type;
                return_type.inner->count = 1;
                check_storage_type_info(compiler, &rtval, return_type);

                add_instruction(
                    compiler,
                    (Instruction){
                        .kind = (req_decl) ? INST_DECL_ASSIGNMENT : INST_ASSIGNMENT,
                        .assignment.left = rtval,
                        .assignment.right =
                            (OperationInst){
                                .kind = OPERATION_C_CALL1,
                                .c_fn_name = NPLIB_DICT_ITEMS,
                                .c_fn_argc = 1,
                                .c_fn_arg = dict_ident,
                            },
                    }
                );
                return rtval;
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
                check_storage_type_info(compiler, &rtval, return_type);

                add_instruction(
                    compiler,
                    (Instruction){
                        .kind = (req_decl) ? INST_DECL_ASSIGNMENT : INST_ASSIGNMENT,
                        .assignment.left = rtval,
                        .assignment.right =
                            (OperationInst){
                                .kind = OPERATION_C_CALL1,
                                .c_fn_name = NPLIB_DICT_KEYS,
                                .c_fn_argc = 1,
                                .c_fn_arg = dict_ident,
                            },
                    }
                );
                return rtval;
            }
            break;
        case 'p':
            if (strcmp(fn_name, "pop") == 0) {
                // TODO: implement default value
                expect_arg_count(compiler, "dict.pop", args, 1);
                check_storage_type_info(compiler, &rtval, val_type);

                StorageIdent* argv =
                    arena_alloc(compiler->arena, sizeof(StorageIdent) * 3);
                argv[0] = dict_ident;
                argv[1] = render_expression(
                    compiler, (StorageHint){.info = key_type}, args->values[0]
                );
                argv[1].reference = true;

                if (req_decl)
                    add_instruction(
                        compiler,
                        (Instruction){
                            .kind = INST_DECLARE_VARIABLE,
                            .declare_variable = rtval,
                        }
                    );
                argv[2] = rtval;
                argv[2].reference = true;

                add_instruction(
                    compiler,
                    (Instruction){
                        .kind = INST_OPERATION,
                        .operation =
                            (OperationInst){
                                .kind = OPERATION_C_CALL,
                                .c_fn_name = NPLIB_DICT_POP,
                                .c_fn_argc = 3,
                                .c_fn_argv = argv,
                            },
                    }
                );
                return rtval;
            }
            break;
            if (strcmp(fn_name, "popitem") == 0) {
                UNIMPLEMENTED(
                    "dict.popitem will not be implemented until tuples are "
                    "implemented"
                );
                return compiler->none_ident;
            }
            break;
        case 'u':
            if (strcmp(fn_name, "update") == 0) {
                // TODO: accept args other than another dict
                expect_arg_count(compiler, "dict.update", args, 1);
                check_storage_type_info(compiler, &rtval, NONE_TYPE);

                StorageIdent* argv =
                    arena_alloc(compiler->arena, sizeof(StorageIdent) * 2);
                argv[0] = dict_ident;
                argv[1] = render_expression(
                    compiler, (StorageHint){.info = dict_ident.info}, args->values[0]
                );

                add_instruction(
                    compiler,
                    (Instruction){
                        .kind = (req_decl) ? INST_DECL_ASSIGNMENT : INST_ASSIGNMENT,
                        .assignment.left = rtval,
                        .assignment.right =
                            (OperationInst){
                                .kind = OPERATION_C_CALL,
                                .c_fn_name = NPLIB_DICT_UPDATE,
                                .c_fn_argc = 2,
                                .c_fn_argv = argv,
                            },
                    }
                );
                return rtval;
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
                check_storage_type_info(compiler, &rtval, return_type);

                add_instruction(
                    compiler,
                    (Instruction){
                        .kind = (req_decl) ? INST_DECL_ASSIGNMENT : INST_ASSIGNMENT,
                        .assignment.left = rtval,
                        .assignment.right =
                            (OperationInst){
                                .kind = OPERATION_C_CALL1,
                                .c_fn_name = NPLIB_DICT_VALUES,
                                .c_fn_argc = 1,
                                .c_fn_arg = dict_ident,
                            },
                    }
                );
                return rtval;
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
    UNREACHABLE();
}

// TODO: parser will need to enforce that builtins dont get defined by the user
static StorageIdent
render_builtin(Compiler* compiler, StorageHint hint, const char* fn_name, Arguments* args)
{
    if (strcmp(fn_name, "print") == 0) {
        return render_builtin_print(compiler, hint, args);
    }
    name_errorf(
        compiler->file_index,
        compiler->current_operation_location,
        "function not found: %s",
        fn_name
    );
    UNREACHABLE();
}

static StorageIdent
convert_to_truthy(Compiler* compiler, StorageIdent id)
{
    switch (id.info.type) {
        case NPTYPE_CSTR:
            return id;
        case NPTYPE_UNSIGNED:
            return id;
        case NPTYPE_BYTE:
            return id;
        case NPTYPE_POINTER:
            return id;
        case NPTYPE_EXCEPTION:
            return id;
        case NPTYPE_FUNCTION: {
            StorageIdent rtval =
                storage_ident_from_hint(compiler, (StorageHint){.info = NPTYPE_POINTER});
            add_instruction(
                compiler,
                (Instruction){
                    .kind = INST_DECL_ASSIGNMENT,
                    .assignment.left = rtval,
                    .assignment.right =
                        (OperationInst){
                            .kind = OPERATION_GET_ATTR,
                            .object = id,
                            .attr = SOURCESTRING("addr"),
                        },
                }
            );
            return rtval;
        }
        case NPTYPE_INT:
            return id;
        case NPTYPE_FLOAT:
            return id;
        case NPTYPE_BOOL:
            return id;
        case NPTYPE_STRING: {
            StorageIdent rtval =
                storage_ident_from_hint(compiler, (StorageHint){.info = INT_TYPE});
            add_instruction(
                compiler,
                (Instruction){
                    .kind = INST_DECL_ASSIGNMENT,
                    .assignment.left = rtval,
                    .assignment.right =
                        (OperationInst){
                            .kind = OPERATION_GET_ATTR,
                            .object = id,
                            .attr = SOURCESTRING("length"),
                        },
                }
            );
            return rtval;
        }
        case NPTYPE_LIST: {
            StorageIdent rtval =
                storage_ident_from_hint(compiler, (StorageHint){.info = INT_TYPE});
            add_instruction(
                compiler,
                (Instruction){
                    .kind = INST_DECL_ASSIGNMENT,
                    .assignment.left = rtval,
                    .assignment.right =
                        (OperationInst){
                            .kind = OPERATION_GET_ATTR,
                            .object = id,
                            .attr = SOURCESTRING("count"),
                        },
                }
            );
            return rtval;
        }
        case NPTYPE_DICT: {
            StorageIdent rtval =
                storage_ident_from_hint(compiler, (StorageHint){.info = INT_TYPE});
            add_instruction(
                compiler,
                (Instruction){
                    .kind = INST_DECL_ASSIGNMENT,
                    .assignment.left = rtval,
                    .assignment.right =
                        (OperationInst){
                            .kind = OPERATION_GET_ATTR,
                            .object = id,
                            .attr = SOURCESTRING("count"),
                        },
                }
            );
            return rtval;
        }
        case NPTYPE_NONE:
            return (StorageIdent){.kind = IDENT_INT_LITERAL, .int_value = 0};
        case NPTYPE_ITER:
            UNIMPLEMENTED("truthy conversion unimplemented for NPTYPE_ITER");
        case NPTYPE_OBJECT: {
            ClassStatement* clsdef = id.info.cls;
            FunctionStatement* fndef = clsdef->object_model_methods[OBJECT_MODEL_BOOL];
            if (!fndef) return id;
            StorageIdent rtval =
                storage_ident_from_hint(compiler, (StorageHint){.info = BOOL_TYPE});
            StorageIdent function_ident =
                render_function_object_copy(compiler, NULL_HINT, fndef);
            bind_self_to_function_object(compiler, id, function_ident);
            add_instruction(
                compiler,
                (Instruction){
                    .kind = INST_DECL_ASSIGNMENT,
                    .assignment.left = rtval,
                    .assignment.right =
                        (OperationInst){
                            .kind = OPERATION_FUNCTION_CALL,
                            .function = function_ident,
                            .args = NULL,
                        },
                }
            );
            return rtval;
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
require_math_lib(Compiler* compiler)
{
    compiler->reqs.libs[LIB_MATH] = true;
}

static StorageIdent
render_object_operation(
    Compiler* compiler, StorageHint hint, Operator op_type, StorageIdent idents[2]
)
{
    bool is_rop;
    bool is_unary;
    FunctionStatement* fndef = find_object_op_function(
        idents[0].info, idents[1].info, op_type, &is_rop, &is_unary
    );
    if (!fndef)
        type_errorf(
            compiler->file_index,
            compiler->current_operation_location,
            "unsupported operand types for `%s`: `%s` and `%s`",
            op_to_cstr(op_type),
            errfmt_type_info(idents[0].info),
            errfmt_type_info(idents[1].info)
        );

    StorageIdent* argv;
    StorageIdent self = idents[0];
    if (!is_unary) {
        argv = arena_alloc(compiler->arena, sizeof(StorageIdent) * 1);
        if (is_rop) {
            argv[0] = idents[0];
            self = idents[1];
        }
        else
            argv[0] = idents[1];
    }

    StorageIdent rtval = storage_ident_from_hint(compiler, hint);
    check_storage_type_info(compiler, &rtval, fndef->sig.return_type);

    StorageIdent function_ident = render_function_object_copy(compiler, NULL_HINT, fndef);
    bind_self_to_function_object(compiler, self, function_ident);
    add_instruction(
        compiler,
        (Instruction){
            .kind = (IS_NULL_IDENTIFIER(hint)) ? INST_DECL_ASSIGNMENT : INST_ASSIGNMENT,
            .assignment.left = rtval,
            .assignment.right =
                (OperationInst){
                    .kind = OPERATION_FUNCTION_CALL,
                    .function = function_ident,
                    .args = argv,
                },
        }
    );
    return rtval;
}

static StorageIdent
render_operation(
    Compiler* compiler, StorageHint hint, Operator op_type, StorageIdent idents[2]
)
{
    if (idents[0].info.type == NPTYPE_OBJECT || idents[1].info.type == NPTYPE_OBJECT)
        return render_object_operation(compiler, hint, op_type, idents);

    TypeInfo resolved_info =
        resolve_operation_type(idents[0].info, idents[1].info, op_type);
    if (resolved_info.type == NPTYPE_UNTYPED) {
        type_errorf(
            compiler->file_index,
            compiler->current_operation_location,
            "unsupported operand types for `%s`: `%s` and `%s`",
            op_to_cstr(op_type),
            errfmt_type_info(idents[0].info),
            errfmt_type_info(idents[1].info)
        );
    }

    StorageIdent rtval = storage_ident_from_hint(compiler, hint);
    check_storage_type_info(compiler, &rtval, resolved_info);
    bool req_decl = IS_NULL_IDENTIFIER(hint);

    StorageIdent left = idents[0];
    StorageIdent right = idents[1];
    StorageIdent* argv;
    const char* lib_function_name;

    switch (op_type) {
        case OPERATOR_LOGICAL_NOT: {
            add_instruction(
                compiler,
                (Instruction){
                    .kind = (req_decl) ? INST_DECL_ASSIGNMENT : INST_ASSIGNMENT,
                    .assignment.left = rtval,
                    .assignment.right =
                        (OperationInst){
                            .kind = OPERATION_INTRINSIC,
                            .op = OPERATOR_BITWISE_NOT,
                            .right = convert_to_truthy(compiler, idents[1]),
                        },
                }
            );
            return rtval;
        }
        case OPERATOR_IS: {
            if (idents[0].info.type == NPTYPE_STRING) {
                // TODO: just string comparison with is an error
                left.reference = true;
                right.reference = true;
            }
            add_instruction(
                compiler,
                (Instruction){
                    .kind = (req_decl) ? INST_DECL_ASSIGNMENT : INST_ASSIGNMENT,
                    .assignment.left = rtval,
                    .assignment.right =
                        (OperationInst){
                            .kind = OPERATION_INTRINSIC,
                            .op = OPERATOR_EQUAL,
                            .left = left,
                            .right = right,
                        },
                }
            );
            return rtval;
        }
        case OPERATOR_PLUS: {
            if (left.info.type == NPTYPE_STRING) {
                lib_function_name = NPLIB_STR_ADD;
                goto lib_function;
            }
            else if (left.info.type == NPTYPE_LIST) {
                lib_function_name = NPLIB_LIST_ADD;
                goto lib_function;
            }
            // break out to default
            break;
        }
        case OPERATOR_DIV:
            // break out to default
            break;
        case OPERATOR_FLOORDIV:
            // break out to default
            break;
        case OPERATOR_MOD:
            if (resolved_info.type == NPTYPE_FLOAT) {
                require_math_lib(compiler);
                lib_function_name = NPLIB_FMOD;
                goto lib_function;
            }
            // break out to default
            break;
        case OPERATOR_POW:
            require_math_lib(compiler);
            lib_function_name = NPLIB_POW;
            goto lib_function;
        case OPERATOR_EQUAL:
            if (left.info.type == NPTYPE_STRING) {
                lib_function_name = NPLIB_STR_EQ;
                goto lib_function;
            }
            // break out to default
            break;
        case OPERATOR_GREATER:
            if (left.info.type == NPTYPE_STRING) {
                lib_function_name = NPLIB_STR_GT;
                goto lib_function;
            }
            // break out to default
            break;
        case OPERATOR_GREATER_EQUAL:
            if (left.info.type == NPTYPE_STRING) {
                lib_function_name = NPLIB_STR_GTE;
                goto lib_function;
            }
            // break out to default
            break;
        case OPERATOR_LESS:
            if (left.info.type == NPTYPE_STRING) {
                lib_function_name = NPLIB_STR_LT;
                goto lib_function;
            }
            // break out to default
            break;
        case OPERATOR_LESS_EQUAL:
            if (left.info.type == NPTYPE_STRING) {
                lib_function_name = NPLIB_STR_LTE;
                goto lib_function;
            }
            // break out to default
            break;
        case OPERATOR_GET_ITEM:
            if (left.info.type == NPTYPE_LIST) {
                if (right.info.type == NPTYPE_SLICE)
                    UNIMPLEMENTED("list slicing unimplemented");

                argv = arena_alloc(compiler->arena, sizeof(OperationInst) * 3);
                argv[0] = left;
                argv[1] = right;
                argv[2] = rtval;
                argv[2].reference = true;

                if (req_decl)
                    add_instruction(
                        compiler,
                        (Instruction){
                            .kind = INST_DECLARE_VARIABLE,
                            .declare_variable = rtval,
                        }
                    );

                add_instruction(
                    compiler,
                    (Instruction){
                        .kind = INST_OPERATION,
                        .operation =
                            (OperationInst){
                                .kind = OPERATION_C_CALL,
                                .c_fn_name = NPLIB_LIST_GET_ITEM,
                                .c_fn_argc = 3,
                                .c_fn_argv = argv,
                            },
                    }
                );
                return rtval;
            }

            else if (left.info.type == NPTYPE_DICT) {
                argv = arena_alloc(compiler->arena, sizeof(OperationInst) * 3);
                argv[0] = left;
                argv[1] = right;
                argv[1].reference = true;
                argv[2] = rtval;
                argv[2].reference = true;

                if (req_decl)
                    add_instruction(
                        compiler,
                        (Instruction){
                            .kind = INST_DECLARE_VARIABLE,
                            .declare_variable = rtval,
                        }
                    );

                add_instruction(
                    compiler,
                    (Instruction){
                        .kind = INST_OPERATION,
                        .operation =
                            (OperationInst){
                                .kind = OPERATION_C_CALL,
                                .c_fn_name = NPLIB_DICT_GET_ITEM,
                                .c_fn_argc = 3,
                                .c_fn_argv = argv,
                            },
                    }
                );

                return rtval;
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

    // default instruction
    add_instruction(
        compiler,
        (Instruction){
            .kind = (req_decl) ? INST_DECL_ASSIGNMENT : INST_ASSIGNMENT,
            .assignment.left = rtval,
            .assignment.right =
                (OperationInst){
                    .kind = OPERATION_INTRINSIC,
                    .left = left,
                    .right = right,
                    .op = op_type,
                },
        }
    );
    return rtval;

lib_function:
    // lib function that accepts 2 args (left/right)
    argv = arena_alloc(compiler->arena, sizeof(OperationInst) * 2);
    argv[0] = left;
    argv[1] = right;
    add_instruction(
        compiler,
        (Instruction){
            .kind = (req_decl) ? INST_DECL_ASSIGNMENT : INST_ASSIGNMENT,
            .assignment.left = rtval,
            .assignment.right =
                (OperationInst){
                    .kind = OPERATION_C_CALL,
                    .c_fn_name = lib_function_name,
                    .c_fn_argc = 2,
                    .c_fn_argv = argv,
                },
        }
    );
    return rtval;
}

static const size_t size_table[NPTYPE_COUNT] = {
    [NPTYPE_INT] = sizeof(NpInt),
    [NPTYPE_FLOAT] = sizeof(NpFloat),
    [NPTYPE_STRING] = sizeof(NpString),
    [NPTYPE_OBJECT] = sizeof(void*),
    [NPTYPE_LIST] = sizeof(NpList*),
    [NPTYPE_DICT] = sizeof(NpDict*),
    [NPTYPE_CONTEXT] = sizeof(NpContext),
    [NPTYPE_FUNCTION] = sizeof(NpFunction),
    [NPTYPE_BOOL] = sizeof(NpBool),
};

static size_t
type_info_sizeof(TypeInfo type_info)
{
    size_t size = size_table[type_info.type];
    if (!size)
        errorf("size data for type `%s` not specified", errfmt_type_info(type_info));
    return size;
}

static StorageIdent
render_empty_enclosure(Compiler* compiler, StorageHint hint, Operand operand)
{
    switch (operand.enclosure->type) {
        case ENCLOSURE_LIST: {
            assert(hint.info.type == NPTYPE_LIST);

            TypeInfo list_content_type = hint.info.inner->types[0];
            const char* rev_sort = sort_cmp_for_type_info(list_content_type, true);
            const char* norm_sort = sort_cmp_for_type_info(list_content_type, false);
            const char* cmp = voidptr_cmp_for_type_info(list_content_type);

            StorageIdent* argv = arena_alloc(compiler->arena, sizeof(StorageIdent) * 4);
            argv[0] = (StorageIdent){
                .kind = IDENT_INT_LITERAL,
                .int_value = type_info_sizeof(list_content_type),
                .info = INT_TYPE,
            };
            argv[1] = (StorageIdent){
                .kind = IDENT_CSTR,
                .cstr = norm_sort,
                .info.type = NPTYPE_POINTER,
            };
            argv[2] = (StorageIdent){
                .kind = IDENT_CSTR,
                .cstr = rev_sort,
                .info.type = NPTYPE_POINTER,
            };
            argv[3] = (StorageIdent){
                .kind = IDENT_CSTR,
                .cstr = cmp,
                .info.type = NPTYPE_POINTER,
            };

            StorageIdent rtval = storage_ident_from_hint(compiler, hint);
            check_storage_type_info(compiler, &rtval, hint.info);

            add_instruction(
                compiler,
                (Instruction){
                    .kind = (IS_NULL_IDENTIFIER(hint)) ? INST_DECL_ASSIGNMENT
                                                       : INST_ASSIGNMENT,
                    .assignment.left = rtval,
                    .assignment.right =
                        (OperationInst){
                            .kind = OPERATION_C_CALL,
                            .c_fn_name = NPLIB_LIST_INIT,
                            .c_fn_argc = 4,
                            .c_fn_argv = argv,
                        },
                }
            );

            return rtval;
        }
        case ENCLOSURE_DICT: {
            assert(hint.info.type == NPTYPE_DICT);

            StorageIdent* argv = arena_alloc(compiler->arena, sizeof(StorageIdent) * 3);
            argv[0] = (StorageIdent){
                .kind = IDENT_INT_LITERAL,
                .int_value = type_info_sizeof(hint.info.inner->types[0]),
                .info = INT_TYPE,
            };
            argv[1] = (StorageIdent){
                .kind = IDENT_INT_LITERAL,
                .int_value = type_info_sizeof(hint.info.inner->types[1]),
                .info = INT_TYPE,
            };
            argv[2] = (StorageIdent){
                .kind = IDENT_CSTR,
                .cstr = voidptr_cmp_for_type_info(hint.info.inner->types[0]),
                .info.type = NPTYPE_POINTER,
            };

            StorageIdent rtval = storage_ident_from_hint(compiler, hint);
            check_storage_type_info(compiler, &rtval, hint.info);

            add_instruction(
                compiler,
                (Instruction){
                    .kind = (IS_NULL_IDENTIFIER(hint)) ? INST_DECL_ASSIGNMENT
                                                       : INST_ASSIGNMENT,
                    .assignment.left = rtval,
                    .assignment.right =
                        (OperationInst){
                            .kind = OPERATION_C_CALL,
                            .c_fn_name = NPLIB_DICT_INIT,
                            .c_fn_argc = 3,
                            .c_fn_argv = argv,
                        },
                }
            );

            return rtval;
        }
        case ENCLOSURE_TUPLE:
            UNIMPLEMENTED("empty tuple rendering unimplemented");
        default:
            UNREACHABLE();
    }
}

static StorageIdent
render_list_literal(Compiler* compiler, StorageHint hint, Operand operand)
{
    // we know the list is not empty

    bool untyped = hint.info.type == NPTYPE_UNTYPED;

    StorageIdent list_element_ident = render_expression(
        compiler,
        (StorageHint){
            .kind = IDENT_CSTR,
            .info = (untyped) ? UNTYPED : hint.info.inner->types[0],
        },
        operand.enclosure->expressions[0]
    );

    if (untyped) {
        TypeInfoInner* inner = arena_alloc(compiler->arena, sizeof(TypeInfoInner));
        inner->types = arena_alloc(compiler->arena, sizeof(TypeInfo));
        inner->types[0] = list_element_ident.info;
        inner->count = 1;
        hint.info.type = NPTYPE_LIST;
        hint.info.inner = inner;
    }

    // TODO: for now we're just going to init an empty list and append
    // everything to it. eventually we should allocate enough room to begin
    // with because we already know the length of the list

    StorageIdent rtval = render_empty_enclosure(compiler, hint, operand);

    StorageIdent* argv = arena_alloc(compiler->arena, sizeof(StorageIdent) * 2);
    argv[0] = rtval;
    argv[1] = list_element_ident;
    argv[1].reference = true;

    size_t i = 1;
    for (;;) {
        add_instruction(
            compiler,
            (Instruction){
                .kind = INST_OPERATION,
                .operation =
                    (OperationInst){
                        .kind = OPERATION_C_CALL,
                        .c_fn_name = NPLIB_LIST_APPEND,
                        .c_fn_argc = 2,
                        .c_fn_argv = argv,
                    },
            }
        );
        if (i == operand.enclosure->expressions_count) break;
        list_element_ident = render_expression(
            compiler, list_element_ident, operand.enclosure->expressions[i++]
        );
    }

    return rtval;
}

static StorageIdent
render_dict_literal(Compiler* compiler, StorageHint hint, Operand operand)
{
    // we know it's not empty at this point

    bool untyped = hint.info.type == NPTYPE_UNTYPED;
    TypeInfo key_info = UNTYPED;
    TypeInfo val_info = UNTYPED;
    if (!untyped) {
        // if we already know the type enforce it
        key_info = hint.info.inner->types[0];
        val_info = hint.info.inner->types[1];
    }

    StorageIdent key_ident = render_expression(
        compiler, (StorageHint){.info = key_info}, operand.enclosure->expressions[0]
    );
    StorageIdent val_ident = render_expression(
        compiler, (StorageHint){.info = val_info}, operand.enclosure->expressions[1]
    );

    // if we don't know the type already then this is the defining statement
    if (untyped) {
        TypeInfoInner* inner = arena_alloc(compiler->arena, sizeof(TypeInfoInner));
        inner->types = arena_alloc(compiler->arena, sizeof(TypeInfo) * 2);
        inner->types[0] = key_ident.info;
        inner->types[1] = val_ident.info;
        inner->count = 2;
        hint.info.type = NPTYPE_DICT;
        hint.info.inner = inner;
    }

    // TODO: better initialization
    StorageIdent rtval = render_empty_enclosure(compiler, hint, operand);
    size_t expression_index = 2;
    for (;;) {
        compile_set_item(compiler, rtval, key_ident, val_ident);
        if (expression_index == operand.enclosure->expressions_count) break;
        key_ident = render_expression(
            compiler, key_ident, operand.enclosure->expressions[expression_index++]
        );
        val_ident = render_expression(
            compiler, val_ident, operand.enclosure->expressions[expression_index++]
        );
    }

    return rtval;
}

static StorageIdent
render_token_operand(Compiler* compiler, StorageHint hint, Operand operand)
{
    TypeInfo resolved = resolve_operand_type(&compiler->tc, operand);
    if (resolved.type == NPTYPE_UNTYPED) {
        // TODO: function to determine if this is a builtin function which
        // cannot be referenced and give a more informative error message.
        type_errorf(
            compiler->file_index,
            *operand.token.loc,
            "unable to resolve the type for token `%s`",
            operand.token.value.data
        );
    }

    StorageIdent rtval;
    StorageIdent value;

    if (resolved.type == NPTYPE_NONE) {
        value = compiler->none_ident;
    }
    else if (operand.token.type == TOK_IDENTIFIER) {
        // TODO: this lookup already happens in resolve_operand_type
        //       -- should find a way to avoid this.
        Symbol* sym = get_symbol(compiler, operand.token.value);
        switch (sym->kind) {
            case SYM_GLOBAL:
                value = storage_ident_from_variable(sym->globalvar);
                break;
            case SYM_VARIABLE:
                value = storage_ident_from_variable(sym->variable);
                break;
            case SYM_FUNCTION:
                value = storage_ident_from_fndef(sym->func);
                break;
            case SYM_CLASS:
                UNIMPLEMENTED("type objects not implemented");
            case SYM_MEMBER:
                UNREACHABLE();
        }
    }
    else if (operand.token.type == TOK_NUMBER) {
        if (resolved.type == NPTYPE_INT)
            value = (StorageIdent){
                .kind = IDENT_INT_LITERAL,
                .int_value = atoi(operand.token.value.data),
                .info = resolved,
            };
        else if (resolved.type == NPTYPE_FLOAT)
            value = (StorageIdent){
                .kind = IDENT_FLOAT_LITERAL,
                .float_value = atof(operand.token.value.data),
                .info = resolved,
            };
        else
            UNIMPLEMENTED("only float and int types are implemented");
    }
    else if (operand.token.type == TOK_STRING) {
        value = (StorageIdent){
            .kind = IDENT_STRING_LITERAL,
            .str_literal_index = str_hm_put(&compiler->str_hm, operand.token.value),
            .info = resolved,
        };
    }
    else if (operand.token.type == TOK_KEYWORD) {
        if (operand.token.kw == KW_TRUE)
            value = (StorageIdent){
                .kind = IDENT_INT_LITERAL,
                .int_value = 1,
                .info = resolved,
            };
        else if (operand.token.kw == KW_FALSE)
            value = (StorageIdent){
                .kind = IDENT_INT_LITERAL,
                .int_value = 0,
                .info = resolved,
            };
        else
            UNREACHABLE();
    }
    else
        UNREACHABLE();

    rtval = storage_ident_from_hint(compiler, hint);
    check_storage_type_info(compiler, &rtval, resolved);
    add_instruction(
        compiler,
        (Instruction){
            .kind = (IS_NULL_IDENTIFIER(hint)) ? INST_DECL_ASSIGNMENT : INST_ASSIGNMENT,
            .assignment.left = rtval,
            .assignment.right =
                (OperationInst){
                    .kind = OPERATION_COPY,
                    .copy = value,
                },
        }
    );
    return rtval;
}

static StorageIdent
render_operand(Compiler* compiler, StorageHint hint, Operand operand)
{
    switch (operand.kind) {
        case OPERAND_ENCLOSURE_LITERAL:
            if (operand.enclosure->expressions_count == 0) {
                if (hint.info.type == NPTYPE_UNTYPED) {
                    // TODO: can probably be caught in the parser
                    type_error(
                        compiler->file_index,
                        compiler->current_stmt_location,
                        "empty containers must have their type annotated when "
                        "initialized"
                    );
                }
                return render_empty_enclosure(compiler, hint, operand);
            }

            switch (operand.enclosure->type) {
                case ENCLOSURE_LIST:
                    return render_list_literal(compiler, hint, operand);
                case ENCLOSURE_DICT:
                    return render_dict_literal(compiler, hint, operand);
                case ENCLOSURE_TUPLE:
                    UNIMPLEMENTED("rendering of tuple enclosure literal unimplemented");
                default:
                    UNREACHABLE();
            }
        case OPERAND_COMPREHENSION:
            UNIMPLEMENTED("comprehension rendering unimplemented");
        case OPERAND_SLICE:
            UNIMPLEMENTED("slice rendering unimplemented");
        case OPERAND_EXPRESSION:
            return render_expression(compiler, hint, operand.expr);
        case OPERAND_TOKEN:
            return render_token_operand(compiler, hint, operand);
        default:
            UNREACHABLE();
    }
}

static StorageIdent*
render_type_hinted_signature_args_to_variables(
    Compiler* compiler, Arguments* args, Signature sig, const char* callable_name
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
            "callable `%s` derives it's type from a type hint and does not "
            "take "
            "keyword "
            "arguments keyword arguments",
            callable_name
        );
    }

    StorageIdent* argv =
        arena_alloc(compiler->arena, sizeof(StorageIdent) * args->values_count);
    for (size_t arg_i = 0; arg_i < args->values_count; arg_i++) {
        argv[arg_i] = render_expression(
            compiler, (StorageHint){.info = sig.types[arg_i]}, args->values[arg_i]
        );
    }
    return argv;
}

static StorageIdent*
render_callable_args_to_variables(
    Compiler* compiler, Arguments* args, Signature sig, const char* callable_name
)
{
    if (sig.params_count == 0) return NULL;
    if (!sig.params) {
        // Signature coming from a type hint, only accepts positional args
        return render_type_hinted_signature_args_to_variables(
            compiler, args, sig, callable_name
        );
    }

    bool params_fulfilled[sig.params_count];
    memset(params_fulfilled, true, sizeof(bool) * args->n_positional);
    memset(
        params_fulfilled + args->n_positional,
        false,
        sizeof(bool) * (sig.params_count - args->n_positional)
    );

    StorageIdent* argv =
        arena_alloc(compiler->arena, sizeof(StorageIdent) * sig.params_count);

    // positional args
    for (size_t arg_i = 0; arg_i < args->n_positional; arg_i++) {
        argv[arg_i] = render_expression(
            compiler, (StorageHint){.info = sig.types[arg_i]}, args->values[arg_i]
        );
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
        argv[param_i] = render_expression(
            compiler, (StorageHint){.info = sig.types[param_i]}, args->values[arg_i]
        );
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
            argv[i] = render_expression(
                compiler,
                (StorageHint){.info = sig.types[i]},
                sig.defaults[i - required_count]
            );
        }
    }

    return argv;
}

static StorageIdent
render_object_method_call(
    Compiler* compiler,
    StorageHint hint,
    StorageIdent self_ident,
    FunctionStatement* fndef,
    Arguments* args
)
{
    // copy the global function variable and bind `self` to the copy
    StorageIdent fn_ident = render_function_object_copy(
        compiler,
        (StorageHint){.info.type = NPTYPE_FUNCTION, .info.sig = &fndef->sig},
        fndef
    );
    bind_self_to_function_object(compiler, self_ident, fn_ident);

    // make the function call
    StorageIdent rtval = storage_ident_from_hint(compiler, hint);
    check_storage_type_info(compiler, &rtval, fndef->sig.return_type);

    add_instruction(
        compiler,
        (Instruction){
            .kind = (IS_NULL_IDENTIFIER(hint)) ? INST_DECL_ASSIGNMENT : INST_ASSIGNMENT,
            .assignment.left = rtval,
            .assignment.right =
                (OperationInst){
                    .kind = OPERATION_FUNCTION_CALL,
                    .function = fn_ident,
                    .args = render_callable_args_to_variables(
                        compiler, args, fndef->sig, fndef->name.data
                    ),
                },
        }
    );

    return rtval;
}

static StorageIdent
render_object_creation(
    Compiler* compiler, StorageHint hint, ClassStatement* clsdef, Arguments* args
)
{
    StorageIdent rtval = storage_ident_from_hint(compiler, hint);
    check_storage_type_info(compiler, &rtval, clsdef->sig.return_type);

    add_instruction(
        compiler,
        (Instruction){
            .kind = (IS_NULL_IDENTIFIER(hint)) ? INST_DECL_ASSIGNMENT : INST_ASSIGNMENT,
            .assignment.left = rtval,
            .assignment.right =
                (OperationInst){
                    .kind = OPERATION_C_CALL1,
                    .c_fn_name = NPLIB_ALLOC,
                    .c_fn_argc = 1,
                    .c_fn_arg =
                        (StorageIdent){
                            .kind = IDENT_INT_LITERAL,
                            .int_value = clsdef->nbytes,
                            .info = INT_TYPE,
                        },
                },
        }
    );

    // use user defined __init__ method
    FunctionStatement* init_def = clsdef->object_model_methods[OBJECT_MODEL_INIT];
    if (init_def) {
        for (size_t i = 0; i < clsdef->sig.defaults_count; i++) {
            size_t def_index = clsdef->sig.params_count - 1 - i;
            add_instruction(
                compiler,
                (Instruction){
                    .kind = INST_OPERATION,
                    .operation =
                        (OperationInst){
                            .kind = OPERATION_SET_ATTR,
                            .object = rtval,
                            .attr = clsdef->sig.params[def_index],
                            .value = render_expression(
                                compiler,
                                (StorageHint){.info = clsdef->sig.types[def_index]},
                                clsdef->sig.defaults[def_index]
                            ),
                        },
                }
            );
        }
        render_object_method_call(
            compiler, (StorageHint){.info = NONE_TYPE}, rtval, init_def, args
        );
        return rtval;
    }

    // default init
    StorageIdent* argv =
        render_callable_args_to_variables(compiler, args, clsdef->sig, clsdef->name.data);
    // set members to variable values
    for (size_t i = 0; i < clsdef->sig.params_count; i++) {
        add_instruction(
            compiler,
            (Instruction){
                .kind = INST_OPERATION,
                .operation =
                    (OperationInst){
                        .kind = OPERATION_SET_ATTR,
                        .object = rtval,
                        .attr = clsdef->sig.params[i],
                        .value = argv[i],
                    },
            }
        );
    }
    return rtval;
}

static StorageIdent
render_call_operation(
    Compiler* compiler, StorageHint hint, StorageIdent fn_ident, Arguments* args
)
{
    // TODO: test calling things that aren't callable
    if (fn_ident.info.type != NPTYPE_FUNCTION) {
        const char* operand =
            (fn_ident.kind = IDENT_VAR) ? fn_ident.var->identifier.data : "operand";
        type_errorf(
            compiler->file_index,
            compiler->current_operation_location,
            "%s is not callable",
            operand
        );
    };

    StorageIdent rtval = storage_ident_from_hint(compiler, hint);
    check_storage_type_info(compiler, &rtval, fn_ident.info.sig->return_type);

    add_instruction(
        compiler,
        (Instruction){
            .kind = (IS_NULL_IDENTIFIER(hint)) ? INST_DECL_ASSIGNMENT : INST_ASSIGNMENT,
            .assignment.left = rtval,
            .assignment.right =
                (OperationInst){
                    .kind = OPERATION_FUNCTION_CALL,
                    .function = fn_ident,
                    .args = render_callable_args_to_variables(
                        compiler,
                        args,
                        *fn_ident.info.sig,
                        (fn_ident.kind == IDENT_VAR) ? fn_ident.var->identifier.data
                                                     : NULL
                    ),
                },
        }
    );
    return rtval;
}

static StorageIdent
render_call_from_operands(
    Compiler* compiler, StorageHint hint, Operand left, Operand right
)
{
    Symbol* sym = get_symbol(compiler, left.token.value);
    if (!sym) return render_builtin(compiler, hint, left.token.value.data, right.args);

    StorageIdent fn_ident;

    switch (sym->kind) {
        case SYM_FUNCTION:
            fn_ident = storage_ident_from_fndef(sym->func);
            break;
        case SYM_VARIABLE:
            fn_ident = storage_ident_from_variable(sym->variable);
            break;
        case SYM_GLOBAL:
            fn_ident = storage_ident_from_variable(sym->globalvar);
            break;
        case SYM_CLASS:
            return render_object_creation(compiler, hint, sym->cls, right.args);
        case SYM_MEMBER:
            UNREACHABLE();
    }
    return render_call_operation(compiler, hint, fn_ident, right.args);
}

// sym var to avoid additional lookup if caller needs the symbol
static TypeInfo
get_class_member_type_info(
    Compiler* compiler,
    ClassStatement* clsdef,
    SourceString member_name,
    Symbol* symbol_out
)
{
    Symbol* sym = symbol_hm_get(&clsdef->scope->hm, member_name);
    if (sym) {
        if (symbol_out) *symbol_out = *sym;
        switch (sym->kind) {
            case SYM_MEMBER:
                return *sym->member_type;
            case SYM_FUNCTION:
                return (TypeInfo){.type = NPTYPE_FUNCTION, .sig = &sym->func->sig};
            default:
                break;
        }
    }

    name_errorf(
        compiler->file_index,
        compiler->current_stmt_location,
        "unknown member `%s` for type `%s`",
        member_name.data,
        clsdef->name.data
    );
    UNREACHABLE();
}

static StorageIdent
render_get_attr_operation(
    Compiler* compiler, StorageHint hint, StorageIdent object, SourceString attr
)
{
    StorageIdent rtval = storage_ident_from_hint(compiler, hint);
    Symbol sym;

    switch (object.info.type) {
        case NPTYPE_OBJECT:
            check_storage_type_info(
                compiler,
                &rtval,
                get_class_member_type_info(compiler, object.info.cls, attr, &sym)
            );
            break;
        default:
            type_errorf(
                compiler->file_index,
                compiler->current_operation_location,
                "getattr not implemented for type `%s`",
                errfmt_type_info(object.info)
            );
    }

    if (object.info.type == NPTYPE_OBJECT && rtval.info.type == NPTYPE_FUNCTION) {
        // copy the global function variable and bind `self` to the copy
        add_instruction(
            compiler,
            (Instruction){
                .kind =
                    (IS_NULL_IDENTIFIER(hint)) ? INST_DECL_ASSIGNMENT : INST_ASSIGNMENT,
                .assignment.left = rtval,
                .assignment.right =
                    (OperationInst){
                        .kind = OPERATION_COPY,
                        .copy = storage_ident_from_fndef(sym.func),
                    },
            }
        );
        bind_self_to_function_object(compiler, object, rtval);
    }
    else
        add_instruction(
            compiler,
            (Instruction){
                .kind =
                    (IS_NULL_IDENTIFIER(hint)) ? INST_DECL_ASSIGNMENT : INST_ASSIGNMENT,
                .assignment.left = rtval,
                .assignment.right =
                    (OperationInst){
                        .kind = OPERATION_GET_ATTR,
                        .object = object,
                        .attr = attr,
                    },
            }
        );

    return rtval;
}

#define IS_SIMPLE_EXPRESSION(expr) ((expr)->operations_count <= 1)

static StorageIdent
render_simple_expression(Compiler* compiler, StorageHint hint, Expression* expr)
{
    assert(IS_SIMPLE_EXPRESSION(expr));

    if (expr->operations_count == 0) {
        Operand operand = expr->operands[0];
        return render_operand(compiler, hint, operand);
    }

    else if (expr->operations_count == 1) {
        Operation operation = expr->operations[0];
        compiler->current_operation_location = *operation.loc;

        if (operation.op_type == OPERATOR_CALL) {
            Operand left = expr->operands[operation.left];
            Operand right = expr->operands[operation.right];
            return render_call_from_operands(compiler, hint, left, right);
        }

        if (operation.op_type == OPERATOR_GET_ATTR) {
            TypeInfo left_type =
                resolve_operand_type(&compiler->tc, expr->operands[operation.left]);
            return render_get_attr_operation(
                compiler,
                hint,
                render_operand(
                    compiler,
                    (StorageHint){.info = left_type},
                    expr->operands[operation.left]
                ),
                expr->operands[operation.right].token.value
            );
        }

        bool is_unary =
            (operation.op_type == OPERATOR_LOGICAL_NOT ||
             operation.op_type == OPERATOR_NEGATIVE ||
             operation.op_type == OPERATOR_BITWISE_NOT);
        StorageIdent operand_idents[2];
        Operand operands[2] = {
            expr->operands[operation.left], expr->operands[operation.right]};
        for (size_t lr = (is_unary) ? 1 : 0; lr < 2; lr++) {
            operand_idents[lr] = render_operand(compiler, NULL_HINT, operands[lr]);
        }
        return render_operation(compiler, hint, operation.op_type, operand_idents);
    }
    UNREACHABLE();
}

typedef struct {
    StorageIdent* previous_operations;
    StorageIdent** lookup_by_operand_index;
    size_t operation_count;
} ExpressionRecord;

#define INIT_EXPRESSION_RECORD(varname, expression_ptr)                                  \
    StorageIdent previous_ops_memory[(expression_ptr)->operations_count - 1];            \
    StorageIdent* lookup_by_operand_memory[(expression_ptr)->operands_count];            \
    memset(                                                                              \
        previous_ops_memory,                                                             \
        0,                                                                               \
        sizeof(StorageIdent) * ((expression_ptr)->operations_count - 1)                  \
    );                                                                                   \
    memset(                                                                              \
        lookup_by_operand_memory,                                                        \
        0,                                                                               \
        sizeof(StorageIdent*) * ((expression_ptr)->operands_count)                       \
    );                                                                                   \
    ExpressionRecord varname = {                                                         \
        .previous_operations = previous_ops_memory,                                      \
        .lookup_by_operand_index = lookup_by_operand_memory,                             \
        .operation_count = 0}

static void
update_expression_record(
    ExpressionRecord* record, StorageIdent current, Operation operation
)
{
    bool is_unary =
        (operation.op_type == OPERATOR_LOGICAL_NOT ||
         operation.op_type == OPERATOR_NEGATIVE ||
         operation.op_type == OPERATOR_BITWISE_NOT);

    StorageIdent** previously_rendered;
    record->previous_operations[record->operation_count] = current;
    StorageIdent* current_ref = record->previous_operations + record->operation_count++;

    if (!is_unary) {
        if ((previously_rendered = record->lookup_by_operand_index + operation.left)) {
            *previously_rendered = current_ref;
        }
        else
            record->lookup_by_operand_index[operation.left] = current_ref;
    }

    if ((previously_rendered = record->lookup_by_operand_index + operation.right)) {
        *previously_rendered = current_ref;
    }
    else
        record->lookup_by_operand_index[operation.right] = current_ref;
}

#define RETURN_EXPRESSION_RESULT(record)                                                 \
    return record.previous_operations[record.operation_count - 1]

StorageIdent
render_expression(Compiler* compiler, StorageHint hint, Expression* expr)
{
    if (IS_SIMPLE_EXPRESSION(expr)) return render_simple_expression(compiler, hint, expr);

    INIT_EXPRESSION_RECORD(record, expr);

    for (size_t i = 0; i < expr->operations_count; i++) {
        Operation operation = expr->operations[i];
        compiler->current_operation_location = *operation.loc;

        StorageHint current_hint = NULL_HINT;
        if (i == expr->operations_count - 1) current_hint = hint;

        switch (operation.op_type) {
            case OPERATOR_CALL: {
                StorageIdent* previous = record.lookup_by_operand_index[operation.left];

                // TODO: evenually migrate builtins to use the standard NpFunction
                // objects
                if (!previous) {
                    update_expression_record(
                        &record,
                        render_call_from_operands(
                            compiler,
                            current_hint,
                            expr->operands[operation.left],
                            expr->operands[operation.right]
                        ),
                        operation
                    );
                    continue;
                }

                StorageIdent left =
                    (previous) ? *previous
                               : render_operand(
                                     compiler, NULL_HINT, expr->operands[operation.left]
                                 );
                update_expression_record(
                    &record,
                    render_call_operation(
                        compiler, current_hint, left, expr->operands[operation.right].args
                    ),
                    operation
                );
                continue;
            }
            case OPERATOR_GET_ATTR: {
                StorageIdent* previous = record.lookup_by_operand_index[operation.left];
                StorageIdent left =
                    (previous) ? *previous
                               : render_operand(
                                     compiler, NULL_HINT, expr->operands[operation.left]
                                 );
                switch (left.info.type) {
                    case NPTYPE_LIST: {
                        if (i == expr->operations_count - 1) {
                            type_error(
                                compiler->file_index,
                                compiler->current_operation_location,
                                "list methods cannot be referenced"
                            );
                        }
                        if (++i == expr->operations_count - 1) current_hint = hint;

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

                        StorageIdent result = render_list_builtin(
                            compiler,
                            current_hint,
                            left,
                            expr->operands[operation.right].token.value.data,
                            expr->operands[next_operation.right].args
                        );
                        update_expression_record(&record, result, operation);
                        update_expression_record(&record, result, next_operation);
                        continue;
                    }
                    case NPTYPE_DICT: {
                        if (i == expr->operations_count - 1) {
                            type_error(
                                compiler->file_index,
                                compiler->current_operation_location,
                                "builtin dict methods cannot be referenced"
                            );
                        }
                        if (++i == expr->operations_count - 1) current_hint = hint;

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

                        StorageIdent result = render_dict_builtin(
                            compiler,
                            current_hint,
                            left,
                            expr->operands[operation.right].token.value.data,
                            expr->operands[next_operation.right].args
                        );
                        update_expression_record(&record, result, operation);
                        update_expression_record(&record, result, next_operation);
                        continue;
                    }
                    case NPTYPE_OBJECT:
                        update_expression_record(
                            &record,
                            render_get_attr_operation(
                                compiler,
                                current_hint,
                                left,
                                expr->operands[operation.right].token.value
                            ),
                            operation
                        );
                        continue;
                    default:
                        UNIMPLEMENTED(
                            "getattr only currently implemented on lists "
                            "dicts "
                            "and "
                            "objects"
                        );
                }
            }
            default: {
                size_t operand_indices[2] = {operation.left, operation.right};
                StorageIdent operand_idents[2] = {0};

                bool is_unary =
                    (operation.op_type == OPERATOR_LOGICAL_NOT ||
                     operation.op_type == OPERATOR_NEGATIVE ||
                     operation.op_type == OPERATOR_BITWISE_NOT);

                for (size_t lr = (is_unary) ? 1 : 0; lr < 2; lr++) {
                    StorageIdent* previous =
                        record.lookup_by_operand_index[operand_indices[lr]];
                    operand_idents[lr] =
                        (previous)
                            ? *previous
                            : render_operand(
                                  compiler, NULL_HINT, expr->operands[operand_indices[lr]]
                              );
                }
                update_expression_record(
                    &record,
                    render_operation(
                        compiler, current_hint, operation.op_type, operand_idents
                    ),
                    operation
                );
                continue;
            }
        }
    }

    RETURN_EXPRESSION_RESULT(record);
}

static void
iterate_scopes_and_update_closure_variables(size_t* closure_size, LexicalScope* scope)
{
    for (size_t i = 0; i < scope->hm.elements_count; i++) {
        Symbol* sym = scope->hm.elements + i;
        if (sym->kind == SYM_VARIABLE && sym->variable->kind == VAR_CLOSURE) {
            sym->variable->closure_offset = *closure_size;
            *closure_size += type_info_sizeof(sym->variable->type_info);
        }
        else if (sym->kind == SYM_FUNCTION)
            iterate_scopes_and_update_closure_variables(closure_size, sym->func->scope);
    }
}

static void
compile_function(Compiler* compiler, FunctionStatement* func)
{
    scope_stack_push(&compiler->scope_stack, func->scope);

    const char* function_name = UNIQUE_ID(compiler);
    StorageIdent fn_variable = {
        .info = (TypeInfo){.type = NPTYPE_FUNCTION, .sig = &func->sig},
        .kind = IDENT_CSTR,
        .cstr = func->ns_ident.data,
    };
    add_instruction(
        compiler,
        (Instruction){
            .kind = INST_DECLARE_VARIABLE,
            .declare_variable = fn_variable,
        }
    );
    add_instruction(
        compiler,
        (Instruction){
            .kind = INST_OPERATION,
            .operation =
                (OperationInst){
                    .kind = OPERATION_SET_ATTR,
                    .object = fn_variable,
                    .attr = SOURCESTRING("addr"),
                    .value = (StorageIdent){.kind = IDENT_CSTR, .cstr = function_name},
                }}
    );
    if (func->scope->kind == SCOPE_CLOSURE_CHILD)
        // copy parent context for closure children so they have access to the closure
        add_instruction(
            compiler,
            (Instruction){
                .kind = INST_OPERATION,
                .operation =
                    (OperationInst){
                        .kind = OPERATION_SET_ATTR,
                        .object = fn_variable,
                        .attr = SOURCESTRING("ctx"),
                        .value = (StorageIdent){.kind = IDENT_CSTR, .cstr = "__ctx__"},
                    }}
        );

    Instruction fndef_inst = {
        .kind = INST_DEFINE_FUNCTION,
        .define_function.function_name = function_name,
        .define_function.signature = func->sig,
        .define_function.var_ident = fn_variable,
    };

    size_t* closure_size;

    COMPILER_ACCUMULATE_INSTRUCTIONS(compiler, fndef_inst.define_function.body)
    {
        if (func->scope->kind == SCOPE_CLOSURE_PARENT) {
            closure_size = arena_alloc(compiler->arena, sizeof(size_t));
            add_instruction(
                compiler,
                (Instruction){
                    .kind = INST_INIT_CLOSURE,
                    .closure_size = closure_size,
                }
            );
        }

        declare_scope_variables(compiler, func->scope);
        if (func->self_param.data) {
            // set `self` variable from context (declared from scope)
            add_instruction(
                compiler,
                (Instruction){
                    .kind = INST_ASSIGNMENT,
                    .assignment.left =
                        (StorageIdent){
                            .kind = IDENT_CSTR,
                            .cstr = func->self_param.data,
                            .info = func->self_type,
                        },
                    .assignment.right =
                        (OperationInst){
                            .kind = OPERATION_GET_ATTR,
                            .object =
                                (StorageIdent){
                                    .kind = IDENT_CSTR,
                                    .cstr = "__ctx__",
                                    .info = CONTEXT_TYPE,
                                },
                            .attr = SOURCESTRING("self"),
                        },
                }
            );
        }

        for (size_t i = 0; i < func->body.stmts_count; i++)
            compile_statement(compiler, func->body.stmts[i]);

        if (func->sig.return_type.type == NPTYPE_NONE)
            // TODO: only do this is the user ommitted the return statement
            add_instruction(
                compiler,
                (Instruction){
                    .kind = INST_RETURN,
                    .return_.rtval = compiler->none_ident,
                    .return_.should_free_closure =
                        func->scope->kind == SCOPE_CLOSURE_PARENT,
                }
            );
    }

    add_instruction(compiler, fndef_inst);

    if (func->scope->kind == SCOPE_CLOSURE_PARENT)
        // now that all types have been resolved we can assign closure offsets
        iterate_scopes_and_update_closure_variables(closure_size, func->scope);

    scope_stack_pop(&compiler->scope_stack);
}

static void
compile_class(Compiler* compiler, ClassStatement* cls)
{
    for (size_t i = 0; i < cls->sig.params_count; i++)
        cls->nbytes += type_info_sizeof(cls->sig.types[i]);

    scope_stack_push(&compiler->scope_stack, cls->scope);

    if (cls->sig.params_count == 0) {
        unspecified_error(
            compiler->file_index,
            compiler->current_stmt_location,
            "class defined without any annotated members"
        );
    }

    Instruction clsdef_inst = {
        .kind = INST_DEFINE_CLASS,
        .define_class.class_name = cls->ns_ident.data,
        .define_class.signature = cls->sig,
    };
    COMPILER_ACCUMULATE_INSTRUCTIONS(compiler, clsdef_inst.define_class.body)
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
                        "only function definitions and annotations are "
                        "currently "
                        "implemented "
                        "within the body of a class"
                    );
            }
        }
    }

    add_instruction(compiler, clsdef_inst);
    scope_stack_pop(&compiler->scope_stack);
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
    Compiler* compiler,
    StorageIdent obj_ident,
    StorageIdent other_ident,
    Operator op_assignment_type
)
{
    assert(obj_ident.info.type == NPTYPE_OBJECT);

    ClassStatement* clsdef = obj_ident.info.cls;
    ObjectModel om = op_assignment_to_object_model(op_assignment_type);
    FunctionStatement* fndef = clsdef->object_model_methods[om];

    if (!fndef) {
        type_errorf(
            compiler->file_index,
            compiler->current_stmt_location,
            "type `%s` does not support `%s` operation",
            errfmt_type_info(obj_ident.info),
            op_to_cstr(op_assignment_type)
        );
    }
    if (!compare_types(other_ident.info, fndef->sig.types[0])) {
        type_errorf(
            compiler->file_index,
            compiler->current_stmt_location,
            "type `%s` does not support `%s` operation with `%s` rtype",
            errfmt_type_info(obj_ident.info),
            op_to_cstr(op_assignment_type),
            errfmt_type_info(other_ident.info)
        );
    }
    if (!compare_types(obj_ident.info, fndef->sig.return_type)) {
        type_errorf(
            compiler->file_index,
            compiler->current_stmt_location,
            "expecting `%s` to return type `%s` but got type `%s`",
            fndef->name,
            errfmt_type_info(obj_ident.info),
            errfmt_type_info(fndef->sig.return_type)
        );
    }

    StorageIdent* argv = arena_alloc(compiler->arena, sizeof(StorageIdent) * 1);
    argv[0] = other_ident;

    StorageIdent fn_ident = render_function_object_copy(compiler, NULL_HINT, fndef);
    bind_self_to_function_object(compiler, obj_ident, fn_ident);
    add_instruction(
        compiler,
        (Instruction){
            .kind = INST_ASSIGNMENT,
            .assignment.left = obj_ident,
            .assignment.right =
                (OperationInst){
                    .kind = OPERATION_FUNCTION_CALL,
                    .function = fn_ident,
                    .args = argv,
                }}
    );
}

static void
compile_set_item(
    Compiler* compiler, StorageIdent container, StorageIdent key, StorageIdent value
)
{
    if (container.info.type == NPTYPE_LIST) {
        StorageIdent* argv = arena_alloc(compiler->arena, sizeof(StorageIdent) * 3);
        argv[0] = container;
        argv[1] = key;
        argv[2] = value;
        argv[2].reference = true;
        add_instruction(
            compiler,
            (Instruction){
                .kind = INST_OPERATION,
                .operation =
                    (OperationInst){
                        .kind = OPERATION_C_CALL,
                        .c_fn_name = NPLIB_LIST_SET_ITEM,
                        .c_fn_argc = 3,
                        .c_fn_argv = argv,
                    },
            }
        );
    }
    else if (container.info.type == NPTYPE_DICT) {
        StorageIdent* argv = arena_alloc(compiler->arena, sizeof(StorageIdent) * 3);
        argv[0] = container;
        argv[1] = key;
        argv[1].reference = true;
        argv[2] = value;
        argv[2].reference = true;
        add_instruction(
            compiler,
            (Instruction){
                .kind = INST_OPERATION,
                .operation =
                    (OperationInst){
                        .kind = OPERATION_C_CALL,
                        .c_fn_name = NPLIB_DICT_SET_ITEM,
                        .c_fn_argc = 3,
                        .c_fn_argv = argv,
                    },
            }
        );
    }
    else {
        UNIMPLEMENTED("setitem not implemented for this data type");
    }
}

static void
compile_complex_assignment(Compiler* compiler, Statement* stmt)
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
    StorageIdent container_ident =
        render_expression(compiler, NULL_HINT, &container_expr);

    if (last_op.op_type == OPERATOR_GET_ITEM) {
        // set key/val expected types
        TypeInfo key_type_info;
        TypeInfo val_type_info;
        switch (container_ident.info.type) {
            case NPTYPE_LIST: {
                key_type_info.type = NPTYPE_INT;
                val_type_info = container_ident.info.inner->types[0];
                break;
            }
            case NPTYPE_DICT: {
                key_type_info = container_ident.info.inner->types[0];
                val_type_info = container_ident.info.inner->types[1];
                break;
            }
            default:
                UNIMPLEMENTED("setitem not implemented for this data type");
        }

        // render key to a variable
        StorageIdent key_ident =
            render_operand(compiler, (StorageHint){.info = key_type_info}, last_operand);

        // render value to a variable
        StorageIdent val_ident;
        StorageHint val_hint = {.info = val_type_info};

        // regular assignment
        if (stmt->assignment->op_type == OPERATOR_ASSIGNMENT)
            val_ident = render_expression(compiler, val_hint, stmt->assignment->value);
        // op-assignment
        else {
            // getitem
            StorageIdent current_val_ident = render_operation(
                compiler,
                (StorageHint){.info = val_type_info},
                OPERATOR_GET_ITEM,
                (StorageIdent[2]){container_ident, key_ident}
            );

            // right side expression
            StorageIdent other_val_ident =
                render_expression(compiler, NULL_HINT, stmt->assignment->value);

            // render __iadd__, __isub__... object model method
            if (current_val_ident.info.type == NPTYPE_OBJECT) {
                compile_object_op_assignment(
                    compiler,
                    current_val_ident,
                    other_val_ident,
                    stmt->assignment->op_type
                );
                return;
            }

            // combine current value with right side expression
            val_ident = render_operation(
                compiler,
                val_hint,
                OP_ASSIGNMENT_TO_OP_TABLE[stmt->assignment->op_type],
                (StorageIdent[2]){current_val_ident, other_val_ident}
            );
        }

        compile_set_item(compiler, container_ident, key_ident, val_ident);
        return;
    }
    else if (last_op.op_type == OPERATOR_GET_ATTR) {
        if (container_ident.info.type != NPTYPE_OBJECT)
            UNIMPLEMENTED("setattr for this type not implemented");

        ClassStatement* clsdef = container_ident.info.cls;
        TypeInfo member_type =
            get_class_member_type_info(compiler, clsdef, last_operand.token.value, NULL);

        if (stmt->assignment->op_type == OPERATOR_ASSIGNMENT) {
            // regular assignment
            add_instruction(
                compiler,
                (Instruction){
                    .kind = INST_OPERATION,
                    .operation =
                        (OperationInst){
                            .kind = OPERATION_SET_ATTR,
                            .object = container_ident,
                            .attr = last_operand.token.value,
                            .value = render_expression(
                                compiler,
                                (StorageHint){.info = member_type},
                                stmt->assignment->value
                            ),
                        }}
            );
            return;
        }
        // op assignment
        else {
            Operator op_type = OP_ASSIGNMENT_TO_OP_TABLE[stmt->assignment->op_type];

            StorageIdent other_val_ident =
                render_expression(compiler, NULL_HINT, stmt->assignment->value);

            StorageIdent current_val_ident = render_get_attr_operation(
                compiler,
                (StorageHint){.info = member_type},
                container_ident,
                last_operand.token.value
            );

            // render op assignment object model method (__iadd__, __isub__, ...)
            if (member_type.type == NPTYPE_OBJECT) {
                compile_object_op_assignment(
                    compiler,
                    current_val_ident,
                    other_val_ident,
                    stmt->assignment->op_type
                );
                return;
            }

            // set_attr(o, getattr(o) `op` other)
            else {
                add_instruction(
                    compiler,
                    (Instruction){
                        .kind = INST_OPERATION,
                        .operation =
                            (OperationInst){
                                .kind = OPERATION_SET_ATTR,
                                .object = container_ident,
                                .attr = last_operand.token.value,
                                .value = render_operation(
                                    compiler,
                                    (StorageHint){.info = member_type},
                                    op_type,
                                    (StorageIdent[2]){current_val_ident, other_val_ident}
                                ),
                            }}
                );
            }
        }
        return;
    }
    else
        UNIMPLEMENTED("complex assignment compilation unimplemented for op type");
}

static void
compile_simple_assignment(Compiler* compiler, Statement* stmt)
{
    SourceString identifier = stmt->assignment->storage->operands[0].token.value;
    Symbol* sym = get_symbol(compiler, identifier);
    if (!sym)
        name_errorf(
            compiler->file_index, stmt->loc, "undefined symbol `%s`", identifier.data
        );
    assert(sym->kind == SYM_VARIABLE || sym->kind == SYM_GLOBAL);

    render_expression(
        compiler, storage_ident_from_variable(sym->variable), stmt->assignment->value
    );
}

static void
compile_simple_op_assignment(Compiler* compiler, Statement* stmt)
{
    SourceString identifier = stmt->assignment->storage->operands[0].token.value;
    Symbol* sym = get_symbol(compiler, identifier);
    if (!sym)
        name_errorf(
            compiler->file_index, stmt->loc, "undefined symbol `%s`", identifier.data
        );

    Operator op_type = OP_ASSIGNMENT_TO_OP_TABLE[stmt->assignment->op_type];
    StorageIdent var_ident = storage_ident_from_variable(sym->variable);
    StorageIdent other_ident =
        render_expression(compiler, NULL_HINT, stmt->assignment->value);

    if (sym->variable->type_info.type == NPTYPE_OBJECT) {
        compile_object_op_assignment(
            compiler, var_ident, other_ident, stmt->assignment->op_type
        );
    }
    else {
        render_operation(
            compiler, var_ident, op_type, (StorageIdent[2]){var_ident, other_ident}
        );
    }
}

static void
compile_assignment(Compiler* compiler, Statement* stmt)
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
compile_annotation(Compiler* compiler, Statement* stmt)
{
    if (scope_stack_peek(&compiler->scope_stack)->kind == SCOPE_CLASS) return;

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

    if (stmt->annotation->initial)
        render_expression(
            compiler,
            storage_ident_from_variable(sym->variable),
            stmt->annotation->initial
        );
}

static void
compile_return_statement(Compiler* compiler, Expression* value)
{
    LexicalScope* scope = scope_stack_peek(&compiler->scope_stack);
    add_instruction(
        compiler,
        (Instruction){
            .kind = INST_RETURN,
            .return_.rtval = render_expression(
                compiler, (StorageHint){.info = scope->func->sig.return_type}, value
            ),
            .return_.should_free_closure = scope->kind == SCOPE_CLOSURE_PARENT,
        }
    );
}

static void
init_semi_scoped_variable(Compiler* compiler, SourceString identifier, TypeInfo type_info)
{
    Symbol* sym = get_symbol(compiler, identifier);
    if (sym->variable->kind != VAR_SEMI_SCOPED) return;

    sym->variable->compiled_name.data = UNIQUE_ID(compiler);
    sym->variable->compiled_name.length = strlen(sym->variable->compiled_name.data);
    sym->variable->type_info = type_info;
    sym->variable->directly_in_scope = true;

    add_instruction(
        compiler,
        (Instruction){
            .kind = INST_DECLARE_VARIABLE,
            .declare_variable = storage_ident_from_variable(sym->variable),
        }
    );
}

static TypeInfo
iterable_of_type(Compiler* compiler, TypeInfo type)
{
    TypeInfo info = {
        .type = NPTYPE_ITER,
        .inner = arena_alloc(compiler->arena, sizeof(TypeInfoInner))};
    info.inner->types = arena_alloc(compiler->arena, sizeof(TypeInfo));
    info.inner->types[0] = type;
    info.inner->count = 1;
    return info;
}

static StorageIdent
convert_to_iterator(Compiler* compiler, StorageHint hint, StorageIdent iterable_ident)
{
    StorageIdent rtval = storage_ident_from_hint(compiler, hint);

    if (iterable_ident.info.type == NPTYPE_LIST) {
        TypeInfo iter_type_info =
            iterable_of_type(compiler, iterable_ident.info.inner->types[0]);
        check_storage_type_info(compiler, &rtval, iter_type_info);

        add_instruction(
            compiler,
            (Instruction){
                .kind =
                    (IS_NULL_IDENTIFIER(hint)) ? INST_DECL_ASSIGNMENT : INST_ASSIGNMENT,
                .assignment.left = rtval,
                .assignment.right =
                    (OperationInst){
                        .kind = OPERATION_C_CALL1,
                        .c_fn_name = NPLIB_LIST_ITER,
                        .c_fn_argc = 1,
                        .c_fn_arg = iterable_ident,
                    },
            }
        );
    }
    else if (iterable_ident.info.type == NPTYPE_DICT) {
        TypeInfo iter_type_info =
            iterable_of_type(compiler, iterable_ident.info.inner->types[0]);
        check_storage_type_info(compiler, &rtval, iter_type_info);

        add_instruction(
            compiler,
            (Instruction){
                .kind =
                    (IS_NULL_IDENTIFIER(hint)) ? INST_DECL_ASSIGNMENT : INST_ASSIGNMENT,
                .assignment.left = rtval,
                .assignment.right =
                    (OperationInst){
                        .kind = OPERATION_C_CALL1,
                        .c_fn_name = NPLIB_DICT_KEYS,
                        .c_fn_argc = 1,
                        .c_fn_arg = iterable_ident,
                    },
            }
        );
    }
    else if (iterable_ident.info.type == NPTYPE_ITER) {
        check_storage_type_info(compiler, &rtval, iterable_ident.info);

        if (IS_NULL_IDENTIFIER(hint))
            rtval = iterable_ident;
        else
            add_instruction(
                compiler,
                (Instruction){
                    .kind = INST_ASSIGNMENT,
                    .assignment.left = rtval,
                    .assignment.right =
                        (OperationInst){
                            .kind = OPERATION_COPY,
                            .copy = iterable_ident,
                        }}
            );
    }
    else
        type_errorf(
            compiler->file_index,
            compiler->current_stmt_location,
            "iterators currently implemented only for lists and dicts, got "
            "`%s`",
            errfmt_type_info(iterable_ident.info)
        );

    return rtval;
}

static void
compile_itid_declarations(Compiler* compiler, TypeInfo iterator_type, ItGroup* its)
{
    if ((its->identifiers_count > 1 || its->identifiers[0].kind == IT_GROUP) &&
        iterator_type.inner->types[0].type != NPTYPE_DICT_ITEMS)
        unspecified_error(
            compiler->file_index,
            compiler->current_stmt_location,
            "multiple iterable identifier variables only supported for "
            "dict.items"
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
compile_for_loop(Compiler* compiler, ForLoopStatement* for_loop)
{
    TypeInfoInner* iterator_inner;
    StorageIdent iterator_ident;

    StorageIdent next_ident;

    Instruction loop_inst = {
        .kind = INST_LOOP,
        .loop.after_label = UNIQUE_ID(compiler),
        .loop.condition = (StorageIdent){.kind = IDENT_INT_LITERAL, .int_value = 1},
    };

    COMPILER_ACCUMULATE_INSTRUCTIONS(compiler, loop_inst.loop.init)
    {
        // create iterator
        iterator_ident = convert_to_iterator(
            compiler,
            NULL_HINT,
            render_expression(compiler, NULL_HINT, for_loop->iterable)
        );
        iterator_inner = iterator_ident.info.inner;

        compile_itid_declarations(compiler, iterator_ident.info, for_loop->it);

        if (iterator_inner->types[0].type == NPTYPE_DICT_ITEMS) {
            // if DictItems unpack to temp DictItems variable first
            // and then unpack into key,val variables later
            next_ident = storage_ident_from_hint(compiler, NULL_HINT);
            set_storage_type_info(compiler, &next_ident, iterator_inner->types[0]);

            add_instruction(
                compiler,
                (Instruction){
                    .kind = INST_DECLARE_VARIABLE,
                    .declare_variable = next_ident,
                }
            );
        }
        else {
            // otherwise unpack directly to the identifier variable
            // (multiple identifiers not currently supported
            Symbol* sym = get_symbol(compiler, for_loop->it->identifiers[0].name);
            next_ident = storage_ident_from_variable(sym->variable);
        }
    }

    COMPILER_ACCUMULATE_INSTRUCTIONS(compiler, loop_inst.loop.before)
    {
        // get next value
        add_instruction(
            compiler,
            (Instruction){
                .kind = INST_ITER_NEXT,
                .iter_next.iter = iterator_ident,
                .iter_next.unpack = next_ident,
            }
        );

        // break if iter.next returned None
        StorageIdent stop =
            storage_ident_from_hint(compiler, (StorageHint){.info = NPTYPE_POINTER});
        add_instruction(
            compiler,
            (Instruction){
                .kind = INST_DECL_ASSIGNMENT,
                .assignment.left = stop,
                .assignment.right =
                    (OperationInst){
                        .kind = OPERATION_GET_ATTR,
                        .object = iterator_ident,
                        .attr = SOURCESTRING("next_data"),
                    }}
        );
        Instruction stop_condition = {
            .kind = INST_IF,
            .if_.condition_ident = stop,
            .if_.negate = true,
        };
        COMPILER_ACCUMULATE_INSTRUCTIONS(compiler, stop_condition.if_.body)
        {
            add_instruction(compiler, (Instruction){.kind = INST_BREAK});
        }
        add_instruction(compiler, stop_condition);

        if (iterator_inner->types[0].type == NPTYPE_DICT_ITEMS) {
            // unpack dict items to it id variable
            StorageIdent key_ident;
            StorageIdent val_ident;

            if (for_loop->it->identifiers_count == 1) {
                Symbol* sym = get_symbol(
                    compiler, for_loop->it->identifiers[0].group->identifiers[0].name
                );
                key_ident = storage_ident_from_variable(sym->variable);
                sym = get_symbol(
                    compiler, for_loop->it->identifiers[0].group->identifiers[1].name
                );
                val_ident = storage_ident_from_variable(sym->variable);
            }
            else {
                Symbol* sym = get_symbol(compiler, for_loop->it->identifiers[0].name);
                key_ident = storage_ident_from_variable(sym->variable);
                sym = get_symbol(compiler, for_loop->it->identifiers[1].name);
                val_ident = storage_ident_from_variable(sym->variable);
            }

            StorageIdent key_val_pointer =
                storage_ident_from_hint(compiler, (StorageHint){.info = POINTER_TYPE});
            add_instruction(
                compiler,
                (Instruction){
                    .kind = INST_DECL_ASSIGNMENT,
                    .assignment.left = key_val_pointer,
                    .assignment.right =
                        (OperationInst){
                            .kind = OPERATION_GET_ATTR,
                            .object = next_ident,
                            .attr = SOURCESTRING("key"),
                        }}
            );
            add_instruction(
                compiler,
                (Instruction){
                    .kind = INST_ASSIGNMENT,
                    .assignment.left = key_ident,
                    .assignment.right =
                        (OperationInst){
                            .kind = OPERATION_DEREF,
                            .ref = key_val_pointer,
                            .info = key_ident.info,
                        }}
            );
            add_instruction(
                compiler,
                (Instruction){
                    .kind = INST_ASSIGNMENT,
                    .assignment.left = key_val_pointer,
                    .assignment.right =
                        (OperationInst){
                            .kind = OPERATION_GET_ATTR,
                            .object = next_ident,
                            .attr = SOURCESTRING("val"),
                        }}
            );
            add_instruction(
                compiler,
                (Instruction){
                    .kind = INST_ASSIGNMENT,
                    .assignment.left = val_ident,
                    .assignment.right =
                        (OperationInst){
                            .kind = OPERATION_DEREF,
                            .ref = key_val_pointer,
                            .info = val_ident.info,
                        }}
            );
        }
    }

    COMPILER_ACCUMULATE_INSTRUCTIONS(compiler, loop_inst.loop.after)
    {
        add_instruction(
            compiler,
            (Instruction){.kind = INST_LABEL, .label = loop_inst.loop.after_label}
        );
    }

    COMPILER_ACCUMULATE_INSTRUCTIONS(compiler, loop_inst.loop.body)
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

    add_instruction(compiler, loop_inst);
}

static void
compile_if(Compiler* compiler, ConditionalStatement* conditional)
{
    const char* exit_ident = NULL;
    bool req_goto =
        (conditional->else_body.stmts_count > 0 || conditional->elifs_count > 0);
    if (req_goto) exit_ident = UNIQUE_ID(compiler);

    // if
    Instruction if_inst = {
        .kind = INST_IF,
        .if_.condition_ident = convert_to_truthy(
            compiler, render_expression(compiler, NULL_HINT, conditional->condition)
        ),
    };
    COMPILER_ACCUMULATE_INSTRUCTIONS(compiler, if_inst.if_.body)
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
    add_instruction(compiler, if_inst);

    // elifs
    for (size_t i = 0; i < conditional->elifs_count; i++) {
        if_inst = (Instruction){
            .kind = INST_IF,
            .if_.condition_ident = convert_to_truthy(
                compiler,
                render_expression(compiler, NULL_HINT, conditional->elifs[i].condition)
            ),
        };
        COMPILER_ACCUMULATE_INSTRUCTIONS(compiler, if_inst.if_.body)
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
        add_instruction(compiler, if_inst);
    }

    // else
    if (conditional->else_body.stmts_count > 0) {
        Instruction else_inst = {.kind = INST_ELSE};
        COMPILER_ACCUMULATE_INSTRUCTIONS(compiler, else_inst.else_)
        {
            for (size_t i = 0; i < conditional->else_body.stmts_count; i++)
                compile_statement(compiler, conditional->else_body.stmts[i]);
        }
        add_instruction(compiler, else_inst);
    }

    if (req_goto)
        add_instruction(compiler, (Instruction){.kind = INST_LABEL, .label = exit_ident});
}

static void
compile_while(Compiler* compiler, WhileStatement* while_stmt)
{
    Instruction loop_inst = {
        .kind = INST_LOOP,
        .loop.condition = (StorageIdent){.kind = IDENT_INT_LITERAL, .int_value = 1},
        .loop.after_label = UNIQUE_ID(compiler),
    };
    COMPILER_ACCUMULATE_INSTRUCTIONS(compiler, loop_inst.loop.before)
    {
        Instruction if_inst = {
            .kind = INST_IF,
            .if_.negate = true,
            .if_.condition_ident = convert_to_truthy(
                compiler, render_expression(compiler, NULL_HINT, while_stmt->condition)
            ),
        };
        COMPILER_ACCUMULATE_INSTRUCTIONS(compiler, if_inst.if_.body)
        {
            add_instruction(compiler, (Instruction){.kind = INST_BREAK});
        }
        add_instruction(compiler, if_inst);
    }
    COMPILER_ACCUMULATE_INSTRUCTIONS(compiler, loop_inst.loop.body)
    {
        for (size_t i = 0; i < while_stmt->body.stmts_count; i++)
            compile_statement(compiler, while_stmt->body.stmts[i]);
    }
    COMPILER_ACCUMULATE_INSTRUCTIONS(compiler, loop_inst.loop.after)
    {
        add_instruction(
            compiler,
            (Instruction){.kind = INST_LABEL, .label = loop_inst.loop.after_label}
        );
    }
    add_instruction(compiler, loop_inst);
}

static void
check_exceptions(Compiler* compiler)
{
    // TODO: replace with callback system in the future
    Instruction if_inst = {
        .kind = INST_IF,
        .if_.condition_ident.kind = IDENT_CSTR,
        .if_.condition_ident.cstr = "global_exception",
        .if_.condition_ident.info = EXCEPTION_TYPE,
    };
    COMPILER_ACCUMULATE_INSTRUCTIONS(compiler, if_inst.if_.body)
    {
        add_instruction(
            compiler, (Instruction){.kind = INST_GOTO, .label = compiler->excepts_goto}
        );
    }
    add_instruction(compiler, if_inst);
}

static void
compile_try(Compiler* compiler, TryStatement* try)
{
    // TODO: ensure fallthrough to finally
    // TODO: if I supported closures this could somehow be packed very neatly
    // into a callback system that would be convenient because you could
    // easily interact with it from C but for now this seems a lot simpler

    const char* old_goto = compiler->excepts_goto;
    const char* finally_goto = UNIQUE_ID(compiler);
    compiler->excepts_goto = UNIQUE_ID(compiler);

    StorageIdent old_excepts_ident =
        storage_ident_from_hint(compiler, (StorageHint){.info = UNSIGNED_TYPE});

    // remember old excepts bitmask
    add_instruction(
        compiler,
        (Instruction){
            .kind = INST_DECL_ASSIGNMENT,
            .assignment.left = old_excepts_ident,
            .assignment.right =
                (OperationInst){
                    .kind = OPERATION_COPY,
                    .copy =
                        (StorageIdent){
                            .kind = IDENT_CSTR,
                            .cstr = NPLIB_CURRENT_EXCEPTS,
                            .info = UNSIGNED_TYPE,
                        },

                }}

    );

    // set new excepts bitmask
    StorageIdent current_excepts_ident = {
        .kind = IDENT_CSTR,
        .cstr = NPLIB_CURRENT_EXCEPTS,
        .info = UNSIGNED_TYPE,
    };
    add_instruction(
        compiler,
        (Instruction){
            .kind = INST_ASSIGNMENT,
            .assignment.left.kind = IDENT_CSTR,
            .assignment.left.cstr = NPLIB_CURRENT_EXCEPTS,
            .assignment.left.info = UNSIGNED_TYPE,
            .assignment.right =
                (OperationInst){
                    .kind = OPERATION_COPY,
                    .copy = (StorageIdent){.kind = IDENT_INT_LITERAL, .int_value = 0},
                },
        }
    );
    for (size_t i = 0; i < try->excepts_count; i++) {
        for (size_t j = 0; j < try->excepts[i].exceptions_count; j++) {
            add_instruction(
                compiler,
                (Instruction){
                    .kind = INST_ASSIGNMENT,
                    .assignment.left = current_excepts_ident,
                    .assignment.right =
                        (OperationInst){
                            .kind = OPERATION_INTRINSIC,
                            .op = OPERATOR_BITWISE_OR,
                            .left = current_excepts_ident,
                            .right =
                                (StorageIdent){
                                    .kind = IDENT_INT_LITERAL,
                                    .int_value = source_string_to_exception_type_flag(
                                        compiler->file_index,
                                        compiler->current_stmt_location,
                                        try->excepts[i].exceptions[j]
                                    ),
                                },
                        }}
            );
        }
    }

    // try block
    for (size_t i = 0; i < try->try_body.stmts_count; i++) {
        compile_statement(compiler, try->try_body.stmts[i]);
        // FIXME: once exceptions are handled with closures and a callback
        // system check exceptions wont be necessary
        check_exceptions(compiler);
    }

    // if an exception hasn't occurred goto finally
    Instruction if_inst = {
        .kind = INST_IF,
        .if_.condition_ident.kind = IDENT_CSTR,
        .if_.condition_ident.cstr = "global_exception",
        .if_.condition_ident.info = EXCEPTION_TYPE,
        .if_.negate = true,
    };
    COMPILER_ACCUMULATE_INSTRUCTIONS(compiler, if_inst.if_.body)
    {
        add_instruction(
            compiler, (Instruction){.kind = INST_GOTO, .label = finally_goto}
        );
    }
    add_instruction(compiler, if_inst);

    // except blocks:

    // add label and declare/assign exception variable
    add_instruction(
        compiler, (Instruction){.kind = INST_LABEL, .label = compiler->excepts_goto}
    );
    StorageIdent exception_ident =
        storage_ident_from_hint(compiler, (StorageHint){.info = EXCEPTION_TYPE});
    add_instruction(
        compiler,
        (Instruction){
            .kind = INST_DECL_ASSIGNMENT,
            .assignment.left = exception_ident,
            .assignment.right =
                (OperationInst){
                    .kind = OPERATION_C_CALL,
                    .c_fn_name = NPLIB_GET_EXCEPTION,
                    .c_fn_argc = 0,
                    .c_fn_argv = NULL,
                }}
    );

    // declare bitmask variable
    StorageIdent except_block_bitmask =
        storage_ident_from_hint(compiler, (StorageHint){.info = UNSIGNED_TYPE});
    add_instruction(
        compiler,
        (Instruction){
            .kind = INST_DECLARE_VARIABLE,
            .declare_variable = except_block_bitmask,
        }
    );

    for (size_t i = 0; i < try->excepts_count; i++) {
        if (try->excepts[i].as.data)
            UNIMPLEMENTED("capturing exception using `as` keyword is not implemented");

        // or together all exception types for this except block
        add_instruction(
            compiler,
            (Instruction){
                .kind = INST_ASSIGNMENT,
                .assignment.left = except_block_bitmask,
                .assignment.right =
                    (OperationInst){
                        .kind = OPERATION_COPY,
                        .copy = (StorageIdent){.kind = IDENT_INT_LITERAL, .int_value = 0},
                    }}
        );
        for (size_t j = 0; j < try->excepts[i].exceptions_count; j++) {
            add_instruction(
                compiler,
                (Instruction){
                    .kind = INST_ASSIGNMENT,
                    .assignment.left = except_block_bitmask,
                    .assignment.right =
                        (OperationInst){
                            .kind = OPERATION_INTRINSIC,
                            .op = OPERATOR_BITWISE_OR,
                            .left = except_block_bitmask,
                            .right =
                                (StorageIdent){
                                    .kind = IDENT_INT_LITERAL,
                                    // FIXME: imprecise location
                                    .int_value = source_string_to_exception_type_flag(
                                        compiler->file_index,
                                        compiler->current_stmt_location,
                                        try->excepts[i].exceptions[j]
                                    ),
                                },
                        }}
            );
        }

        // compare bitmask
        StorageIdent if_cond_ident =
            storage_ident_from_hint(compiler, (StorageHint){.info = UNSIGNED_TYPE});
        StorageIdent exc_type =
            storage_ident_from_hint(compiler, (StorageHint){.info = UNSIGNED_TYPE});
        add_instruction(
            compiler,
            (Instruction){
                .kind = INST_DECL_ASSIGNMENT,
                .assignment.left = exc_type,
                .assignment.right =
                    (OperationInst){
                        .kind = OPERATION_GET_ATTR,
                        .object = exception_ident,
                        .attr = SOURCESTRING("type"),
                    }}
        );
        add_instruction(
            compiler,
            (Instruction){
                .kind = INST_DECL_ASSIGNMENT,
                .assignment.left = if_cond_ident,
                .assignment.right =
                    (OperationInst){
                        .kind = OPERATION_INTRINSIC,
                        .op = OPERATOR_BITWISE_AND,
                        .left = exc_type,
                        .right = except_block_bitmask,
                    }}
        );
        if_cond_ident = convert_to_truthy(compiler, if_cond_ident);

        Instruction if_inst = {
            .kind = INST_IF,
            .if_.condition_ident = if_cond_ident,
        };
        COMPILER_ACCUMULATE_INSTRUCTIONS(compiler, if_inst.if_.body)
        {
            for (size_t j = 0; j < try->excepts[i].body.stmts_count; j++)
                compile_statement(compiler, try->excepts[i].body.stmts[j]);
            add_instruction(
                compiler, (Instruction){.kind = INST_GOTO, .label = finally_goto}
            );
        }
        add_instruction(compiler, if_inst);
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
            .assignment.right =
                (OperationInst){
                    .kind = OPERATION_COPY,
                    .copy = old_excepts_ident,
                }}
    );
    compiler->excepts_goto = old_goto;
}

static void
compile_assert(Compiler* compiler, Expression* value)
{
    Instruction if_inst = {
        .kind = INST_IF,
        .if_.condition_ident =
            convert_to_truthy(compiler, render_expression(compiler, NULL_HINT, value)),
        .if_.negate = true,
    };
    COMPILER_ACCUMULATE_INSTRUCTIONS(compiler, if_inst.if_.body)
    {
        add_instruction(
            compiler,
            (Instruction){
                .kind = INST_OPERATION,
                .operation =
                    (OperationInst){
                        .kind = OPERATION_C_CALL1,
                        .c_fn_name = NPLIB_ASSERTION_ERROR,
                        .c_fn_argc = 1,
                        .c_fn_arg =
                            (StorageIdent){
                                .kind = IDENT_INT_LITERAL,
                                .int_value = compiler->current_stmt_location.line,
                            },
                    }}
        );
    }
    add_instruction(compiler, if_inst);
}

static void
compile_statement(Compiler* compiler, Statement* stmt)
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
            render_expression(compiler, NULL_HINT, stmt->expr);
            break;
        }
        case STMT_NO_OP:
            // might want to put in a `;` but for now just skipping
            break;
        case STMT_EOF:
            break;
        case STMT_RETURN: {
            // TODO: make sure return statements are only allowed inside
            // functions
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
declare_scope_variables(Compiler* compiler, LexicalScope* scope)
{
    for (size_t i = 0; i < scope->hm.elements_count; i++) {
        Symbol s = scope->hm.elements[i];
        if (s.kind == SYM_VARIABLE) {
            switch (s.variable->kind) {
                case VAR_SEMI_SCOPED:
                    // semi scoped var is managed in the compiler
                    continue;
                default:
                    // writer can decide how to handle different variable types
                    add_instruction(
                        compiler,
                        (Instruction){
                            .kind = INST_DECLARE_VARIABLE,
                            .declare_variable.kind = IDENT_VAR,
                            .declare_variable.var = s.variable,
                        }
                    );
                    break;
            }
        }
    }
}

static void
str_hm_rehash(StringHashmap* hm)
{
    for (size_t i = 0; i < hm->count; i++) {
        SourceString str = hm->elements[i];
        uint64_t hash = hash_bytes((void*)str.data, str.length);
        size_t probe = hash % (hm->capacity * 2);
        for (;;) {
            int index = hm->lookup[probe];
            if (index == -1) {
                hm->lookup[probe] = i;
                break;
            }
            if (probe == hm->capacity * 2)
                probe = 0;
            else
                probe++;
        }
    }
}

static void
str_hm_grow(StringHashmap* hm)
{
    if (hm->capacity == 0)
        hm->capacity = 16;
    else
        hm->capacity *= 2;
    hm->elements = realloc(hm->elements, sizeof(NpString) * hm->capacity);
    hm->lookup = realloc(hm->lookup, sizeof(int) * 2 * hm->capacity);
    if (!hm->elements || !hm->lookup) error("out of memory");
    memset(hm->lookup, -1, sizeof(int) * 2 * hm->capacity);
    str_hm_rehash(hm);
}

static size_t
str_hm_put(StringHashmap* hm, SourceString element)
{
    if (hm->count == hm->capacity) str_hm_grow(hm);
    uint64_t hash = hash_bytes((void*)element.data, element.length);
    size_t probe = hash % (hm->capacity * 2);
    for (;;) {
        int index = hm->lookup[probe];
        if (index == -1) {
            hm->lookup[probe] = hm->count;
            hm->elements[hm->count] = element;
            return hm->count++;
        }
        SourceString existing = hm->elements[index];
        if (SOURCESTRING_EQ(element, existing)) return (size_t)index;
        if (probe == hm->capacity * 2)
            probe = 0;
        else
            probe++;
    }
}

static void
str_hm_free(StringHashmap* hm)
{
    free(hm->elements);
    free(hm->lookup);
}
