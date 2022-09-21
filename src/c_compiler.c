#include "c_compiler.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "np_hash.h"
// TODO: once this module is finished we should find a way to not rely on lexer_helpers
#include "c_compiler_helpers.h"
#include "diagnostics.h"
#include "lexer_helpers.h"
#include "not_python.h"
#include "object_model.h"
#include "syntax.h"
#include "type_checker.h"

// TODO: some standard way to implement name mangling will be needed at some point
// TODO: tracking if variables have been initialized before use and warning if not

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
    CompilerSection forward;
    CompilerSection variable_declarations;
    CompilerSection struct_declarations;
    CompilerSection function_declarations;
    CompilerSection function_definitions;
    CompilerSection init_module_function;
    CompilerSection main_function;
    size_t unique_vars_counter;
    StringBuilder sb;
} C_Compiler;

// TODO: unique vars should probably be namespaced by module in the future

#define UNIQUE_VAR_LENGTH 12

// TODO: I'd prefer a change that allows this macro to accept `;` at the end
#define GENERATE_UNIQUE_VAR_NAMES(compiler_ptr, count, name)                             \
    char name[count][UNIQUE_VAR_LENGTH];                                                 \
    for (size_t i = 0; i < count; i++) {                                                 \
        sprintf(name[i], "NP_var%zu", compiler_ptr->unique_vars_counter++);              \
    }

#define GENERATE_UNIQUE_VAR_NAME(compiler_ptr, name)                                     \
    char name[UNIQUE_VAR_LENGTH];                                                        \
    sprintf(name, "NP_var%zu", compiler_ptr->unique_vars_counter++)

#define WRITE_UNIQUE_VAR_NAME(compiler_ptr, dest)                                        \
    sprintf(dest, "NP_var%zu", compiler_ptr->unique_vars_counter++)

#define STRING_CONSTANTS_TABLE_NAME "NOT_PYTHON_STRING_CONSTANTS"

static const char* SORT_CMP_TABLE[PYTYPE_COUNT] = {
    [PYTYPE_INT] = "pyint_sort_fn",
    [PYTYPE_FLOAT] = "pyfloat_sort_fn",
    [PYTYPE_BOOL] = "pybool_sort_fn",
    [PYTYPE_STRING] = "ptstr_sort_fn",
};

static const char* REVERSE_SORT_CMP_TABLE[PYTYPE_COUNT] = {
    [PYTYPE_INT] = "pyint_sort_fn_rev",
    [PYTYPE_FLOAT] = "pyfloat_sort_fn_rev",
    [PYTYPE_BOOL] = "pybool_sort_fn_rev",
    [PYTYPE_STRING] = "ptstr_sort_fn_rev",
};

static const char*
sort_cmp_for_type_info(TypeInfo type_info, bool reversed)
{
    const char* rtval;
    if (reversed)
        rtval = REVERSE_SORT_CMP_TABLE[type_info.type];
    else
        rtval = SORT_CMP_TABLE[type_info.type];
    if (!rtval) {
        // TODO: error message
        fprintf(stderr, "ERROR: sorting comparison function not implemented\n");
        exit(1);
    }
    return rtval;
}

static const char* VOIDPTR_CMP_TABLE[PYTYPE_COUNT] = {
    [PYTYPE_INT] = "void_int_eq",
    [PYTYPE_FLOAT] = "void_float_eq",
    [PYTYPE_BOOL] = "void_bool_eq",
    [PYTYPE_STRING] = "void_str_eq",
};

static const char* CMP_TABLE[PYTYPE_COUNT] = {
    [PYTYPE_INT] = "int_eq",
    [PYTYPE_FLOAT] = "float_eq",
    [PYTYPE_BOOL] = "bool_eq",
    [PYTYPE_STRING] = "str_eq",
};

static const char*
voidptr_cmp_for_type_info(TypeInfo type_info)
{
    const char* cmp_for_type = VOIDPTR_CMP_TABLE[type_info.type];
    if (!cmp_for_type) {
        // TODO: error message
        fprintf(stderr, "ERROR: comparison function not implemented\n");
        exit(1);
    }
    return cmp_for_type;
}

static const char*
cmp_for_type_info(TypeInfo type_info)
{
    const char* cmp_for_type = CMP_TABLE[type_info.type];
    if (!cmp_for_type) {
        // TODO: error message
        fprintf(stderr, "ERROR: comparison function not implemented\n");
        exit(1);
    }
    return cmp_for_type;
}

// TODO: maybe C_Variable is a better name
typedef struct {
    CompilerSection* section;
    TypeInfo type_info;
    char* user_defined_variable_name;
    char* variable_name;
    bool is_declared;
    Location* loc;  // NULL for intermediate assignments
} C_Assignment;

static void
set_assignment_type_info(
    C_Compiler* compiler, C_Assignment* assignment, TypeInfo type_info
)
{
    assert(assignment->type_info.type < PYTYPE_COUNT);
    if (assignment->type_info.type != PYTYPE_UNTYPED &&
        !compare_types(type_info, assignment->type_info)) {
        // TODO: some kind of check if this is safe to cast such
        //      ex: if expecting a float, and actually got an int, it's probably safe to
        //      just cast to float

        static const size_t buflen = 1024;
        char expected[buflen];
        char actual[buflen];
        render_type_info_human_readable(assignment->type_info, expected, buflen);
        render_type_info_human_readable(type_info, actual, buflen);

        if (assignment->loc) {
            type_errorf(
                compiler->file_index,
                *assignment->loc,
                "trying to assign type `%s` to variable `%s` of type `%s`",
                actual,
                assignment->user_defined_variable_name,
                expected
            );
        }
        else {
            unimplemented_errorf(
                compiler->file_index,
                compiler->current_stmt_location,
                "inconsistent typing: expecting `%s` got `%s`",
                expected,
                actual
            );
        }
    }
    else {
        assignment->type_info = type_info;
    }
}

static void compile_statement(
    C_Compiler* compiler, CompilerSection* section, Statement* stmt
);

static void render_expression_assignment(
    C_Compiler* compiler, C_Assignment* assignment, Expression* expr
);

static void convert_assignment_to_string(
    C_Compiler* compiler, C_Assignment assignment_to_convert, C_Assignment* destination
);

static void
prepare_c_assignment_for_rendering(C_Compiler* compiler, C_Assignment* assignment)
{
    if (assignment->variable_name) {
        if (!assignment->is_declared) {
            if (assignment->type_info.type == PYTYPE_UNTYPED) UNTYPED_ERROR();
            write_type_info_to_section(
                assignment->section, &compiler->sb, assignment->type_info
            );
            assignment->is_declared = true;
        }
        write_to_section(assignment->section, assignment->variable_name);
        write_to_section(assignment->section, " = ");
    }
}

static char*
simple_operand_repr(C_Compiler* compiler, Operand operand)
{
    if (operand.token.type == TOK_IDENTIFIER) {
        // TODO: None should probably be a keyword
        if (strcmp(operand.token.value, "None") == 0) return "NULL";
        // TODO: should consider a way to avoid this lookup because it happens often
        LexicalScope* scope = scope_stack_peek(&compiler->scope_stack);
        Symbol* sym = symbol_hm_get(&scope->hm, operand.token.value);
        switch (sym->kind) {
            case SYM_SEMI_SCOPED:
                return sym->semi_scoped->current_id;
            case SYM_VARIABLE:
                return sym->variable->ns_ident;
            case SYM_FUNCTION:
                return sym->func->ns_ident;
            case SYM_CLASS:
                return sym->cls->ns_ident;
        }
        UNREACHABLE("unexpected symbol kind")
    }
    else if (operand.token.type == TOK_NUMBER) {
        return operand.token.value;
    }
    else if (operand.token.type == TOK_STRING) {
        size_t index = str_hm_put_cstr(&compiler->str_hm, operand.token.value);
        char strindex[10];
        sprintf(strindex, "%zu", index);
        return sb_build(
            &compiler->sb, STRING_CONSTANTS_TABLE_NAME, "[", strindex, "]", NULL
        );
    }
    else if (operand.token.type == TOK_KEYWORD) {
        if (operand.token.kw == KW_TRUE)
            return "true";
        else if (operand.token.kw == KW_FALSE)
            return "false";
        else
            UNREACHABLE("unexpected simple operand token type");
    }
    else
        UNREACHABLE("unexpected simple operand");
}

static void
render_simple_operand(C_Compiler* compiler, C_Assignment* assignment, Operand operand)
{
    set_assignment_type_info(
        compiler, assignment, resolve_operand_type(&compiler->tc, operand)
    );
    prepare_c_assignment_for_rendering(compiler, assignment);
    write_to_section(assignment->section, simple_operand_repr(compiler, operand));
    write_to_section(assignment->section, ";\n");
}

static void
render_empty_enclosure(
    C_Compiler* compiler, C_Assignment* enclosure_assignment, Operand operand
)
{
    switch (operand.enclosure->type) {
        case ENCLOSURE_LIST:
            prepare_c_assignment_for_rendering(compiler, enclosure_assignment);
            write_many_to_section(
                enclosure_assignment->section,
                "LIST_INIT(",
                type_info_to_c_syntax(
                    &compiler->sb, enclosure_assignment->type_info.inner->types[0]
                ),
                ");\n",
                NULL
            );
            break;
        case ENCLOSURE_DICT:
            prepare_c_assignment_for_rendering(compiler, enclosure_assignment);
            TypeInfo key_type = enclosure_assignment->type_info.inner->types[0];
            write_many_to_section(
                enclosure_assignment->section,
                "DICT_INIT(",
                type_info_to_c_syntax(&compiler->sb, key_type),
                ", ",
                type_info_to_c_syntax(
                    &compiler->sb, enclosure_assignment->type_info.inner->types[1]
                ),
                ", ",
                voidptr_cmp_for_type_info(key_type),
                ");\n",
                NULL
            );
            break;
        case ENCLOSURE_TUPLE:
            UNIMPLEMENTED("empty tuple rendering unimplemented");
        default:
            UNREACHABLE("end of render empty enclosure switch");
    }
}

static void
render_list_literal(
    C_Compiler* compiler, C_Assignment* enclosure_assignment, Operand operand
)
{
    GENERATE_UNIQUE_VAR_NAME(compiler, expression_variable);
    C_Assignment expression_assignment = {
        .is_declared = false,
        .section = enclosure_assignment->section,
        .variable_name = expression_variable};
    render_expression_assignment(
        compiler, &expression_assignment, operand.enclosure->expressions[0]
    );

    TypeInfoInner* inner = arena_alloc(compiler->arena, sizeof(TypeInfoInner));
    inner->types = arena_alloc(compiler->arena, sizeof(TypeInfo));
    inner->types[0] = expression_assignment.type_info;
    inner->count = 1;
    TypeInfo enclosure_type_info = {.type = PYTYPE_LIST, .inner = inner};
    set_assignment_type_info(compiler, enclosure_assignment, enclosure_type_info);

    // TODO: for now we're just going to init an empty list and append everything
    // to it. eventually we should allocate enough room to begin with because we
    // already know the length of the list
    render_empty_enclosure(compiler, enclosure_assignment, operand);
    size_t i = 1;
    for (;;) {
        write_many_to_section(
            enclosure_assignment->section,
            "LIST_APPEND(",
            enclosure_assignment->variable_name,
            ", ",
            type_info_to_c_syntax(&compiler->sb, expression_assignment.type_info),
            ", ",
            expression_assignment.variable_name,
            ");\n",
            NULL
        );
        if (i == operand.enclosure->expressions_count) break;
        render_expression_assignment(
            compiler, &expression_assignment, operand.enclosure->expressions[i++]
        );
    }
}

static void
render_dict_literal(
    C_Compiler* compiler, C_Assignment* enclosure_assignment, Operand operand
)
{
    GENERATE_UNIQUE_VAR_NAME(compiler, key_variable);
    C_Assignment key_assignment = {
        .is_declared = false,
        .section = enclosure_assignment->section,
        .variable_name = key_variable};
    render_expression_assignment(
        compiler, &key_assignment, operand.enclosure->expressions[0]
    );
    GENERATE_UNIQUE_VAR_NAME(compiler, val_variable);
    C_Assignment val_assignment = {
        .is_declared = false,
        .section = enclosure_assignment->section,
        .variable_name = val_variable};
    render_expression_assignment(
        compiler, &val_assignment, operand.enclosure->expressions[1]
    );

    TypeInfoInner* inner = arena_alloc(compiler->arena, sizeof(TypeInfoInner));
    inner->types = arena_alloc(compiler->arena, sizeof(TypeInfo) * 2);
    inner->types[0] = key_assignment.type_info;
    inner->types[1] = val_assignment.type_info;
    inner->count = 2;
    TypeInfo enclosure_type_info = {.type = PYTYPE_DICT, .inner = inner};
    set_assignment_type_info(compiler, enclosure_assignment, enclosure_type_info);

    // TODO: better initialization
    render_empty_enclosure(compiler, enclosure_assignment, operand);
    size_t expression_index = 2;
    for (;;) {
        write_many_to_section(
            enclosure_assignment->section,
            "dict_set_item(",
            enclosure_assignment->variable_name,
            ", &",
            key_assignment.variable_name,
            ", &",
            val_assignment.variable_name,
            ");\n",
            NULL
        );
        if (expression_index == operand.enclosure->expressions_count) break;
        render_expression_assignment(
            compiler, &key_assignment, operand.enclosure->expressions[expression_index++]
        );
        render_expression_assignment(
            compiler, &val_assignment, operand.enclosure->expressions[expression_index++]
        );
    }
}

static void
render_enclosure_literal(
    C_Compiler* compiler, C_Assignment* enclosure_assignment, Operand operand
)
{
    if (operand.enclosure->expressions_count == 0) {
        if (enclosure_assignment->type_info.type == PYTYPE_UNTYPED) {
            type_error(
                compiler->file_index,
                compiler->current_stmt_location,
                "empty containers must have their type annotated when initialized"
            );
        }
        render_empty_enclosure(compiler, enclosure_assignment, operand);
        return;
    }

    if (enclosure_assignment->variable_name == NULL) {
        // TODO: python allows this but I'm not sure it makes sense for us to allow this
        unimplemented_error(
            compiler->file_index,
            compiler->current_stmt_location,
            "enclosures are currently required to be assigned to a variable"
        );
    }

    switch (operand.enclosure->type) {
        case ENCLOSURE_LIST:
            render_list_literal(compiler, enclosure_assignment, operand);
            break;
        case ENCLOSURE_DICT:
            render_dict_literal(compiler, enclosure_assignment, operand);
            break;
        case ENCLOSURE_TUPLE:
            UNIMPLEMENTED("rendering of tuple enclosure literal unimplemented");
        default:
            UNREACHABLE("enclosure literal default case unreachable")
    }
}

// -1 on bad kwd
// TODO: should check if parser even allows this to happen in the future
static int
index_of_kwarg(Signature sig, char* kwd)
{
    for (size_t i = 0; i < sig.params_count; i++) {
        if (strcmp(kwd, sig.params[i]) == 0) return i;
    }
    return -1;
}

static void
render_callable_args_to_variables(
    C_Compiler* compiler,
    CompilerSection* section,
    Arguments* args,
    Signature sig,
    char* callable_name,
    char** variable_names,
    bool variables_declared
)
{
    C_Assignment assignments[sig.params_count];
    memset(assignments, 0, sizeof(C_Assignment) * sig.params_count);

    for (size_t i = 0; i < sig.params_count; i++) {
        assignments[i].section = section;
        assignments[i].type_info = sig.types[i];
        assignments[i].variable_name = variable_names[i];
        assignments[i].is_declared = variables_declared;
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
    for (size_t arg_i = 0; arg_i < args->n_positional; arg_i++)
        render_expression_assignment(compiler, assignments + arg_i, args->values[arg_i]);

    // parse kwargs
    for (size_t arg_i = args->n_positional; arg_i < args->values_count; arg_i++) {
        char* kwd = args->kwds[arg_i - args->n_positional];
        int param_i = index_of_kwarg(sig, kwd);
        if (param_i < 0) {
            type_errorf(
                compiler->file_index,
                compiler->current_operation_location,
                "callable `%s` was given an unexpected keyword argument `%s`",
                callable_name,
                kwd
            );
        }
        params_fulfilled[param_i] = true;

        render_expression_assignment(
            compiler, assignments + param_i, args->values[arg_i]
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
                sig.params[i]
            );
        }
    }

    // fill in any unprovided values with their default expressions
    for (size_t i = required_count; i < sig.params_count; i++) {
        if (params_fulfilled[i])
            continue;
        else
            render_expression_assignment(
                compiler, assignments + i, sig.defaults[i - required_count]
            );
    }
}

static void
render_function_call(
    C_Compiler* compiler,
    C_Assignment* assignment,
    FunctionStatement* fndef,
    Arguments* args
)
{
    // intermediate variables to store args into
    GENERATE_UNIQUE_VAR_NAMES(compiler, fndef->sig.params_count, param_vars)
    char* variable_names[fndef->sig.params_count];
    for (size_t i = 0; i < fndef->sig.params_count; i++)
        variable_names[i] = param_vars[i];

    render_callable_args_to_variables(
        compiler,
        assignment->section,
        args,
        fndef->sig,
        fndef->name,
        variable_names,
        false
    );

    // write the call statement
    set_assignment_type_info(compiler, assignment, fndef->sig.return_type);
    prepare_c_assignment_for_rendering(compiler, assignment);
    write_to_section(assignment->section, fndef->ns_ident);
    write_to_section(assignment->section, "(");
    for (size_t arg_i = 0; arg_i < fndef->sig.params_count; arg_i++) {
        if (arg_i > 0) write_to_section(assignment->section, ", ");
        write_to_section(assignment->section, param_vars[arg_i]);
    }
    write_to_section(assignment->section, ");\n");
}

static void
render_default_object_string_represntation(
    C_Compiler* compiler,
    ClassStatement* clsdef,
    C_Assignment object_assignment,
    C_Assignment* destination
)
{
    // create a python string that looks like:
    //  ClassName(param1=param1_value, param2=param2_value...)
    if (!clsdef->fmtstr)
        // default fmt string expects all params to be first convereted into cstr
        clsdef->fmtstr = create_default_object_fmt_str(compiler->arena, clsdef);

    GENERATE_UNIQUE_VAR_NAMES(compiler, clsdef->sig.params_count, members_as_str);
    C_Assignment as_str_assignments[clsdef->sig.params_count];
    for (size_t i = 0; i < clsdef->sig.params_count; i++) {
        C_Assignment fake_member_assignment = {
            .section = object_assignment.section,
            .type_info = clsdef->sig.types[i],
            .variable_name = sb_build(
                &compiler->sb,
                object_assignment.variable_name,
                "->",
                clsdef->sig.params[i],
                NULL
            )};
        C_Assignment* to_string = as_str_assignments + i;
        to_string->section = object_assignment.section;
        to_string->type_info.type = PYTYPE_STRING;
        to_string->is_declared = false;
        to_string->variable_name = members_as_str[i];
        convert_assignment_to_string(compiler, fake_member_assignment, to_string);
    }

    prepare_c_assignment_for_rendering(compiler, destination);
    write_many_to_section(destination->section, "str_fmt(\"", clsdef->fmtstr, "\"", NULL);
    for (size_t i = 0; i < clsdef->sig.params_count; i++)
        write_many_to_section(
            destination->section, ", np_str_to_cstr(", members_as_str[i], ")", NULL
        );
    write_to_section(destination->section, ");\n");
}

static void
convert_object_to_str(
    C_Compiler* compiler, C_Assignment object_assignment, C_Assignment* destination
)
{
    FunctionStatement* user_defined_str =
        object_assignment.type_info.cls->object_model_methods[OBJECT_MODEL_STR];
    if (user_defined_str) {
        prepare_c_assignment_for_rendering(compiler, destination);
        write_many_to_section(
            destination->section,
            user_defined_str->ns_ident,
            "(",
            object_assignment.variable_name,
            ")",
            NULL
        );
    }
    else
        render_default_object_string_represntation(
            compiler, object_assignment.type_info.cls, object_assignment, destination
        );
}

static void
convert_assignment_to_string(
    C_Compiler* compiler, C_Assignment assignment_to_convert, C_Assignment* destination
)
{
    if (assignment_to_convert.type_info.type == PYTYPE_STRING) return;

    switch (assignment_to_convert.type_info.type) {
        case PYTYPE_INT:
            prepare_c_assignment_for_rendering(compiler, destination);
            write_many_to_section(
                destination->section,
                "np_int_to_str(",
                assignment_to_convert.variable_name,
                ");\n",
                NULL
            );
            return;
        case PYTYPE_FLOAT:
            prepare_c_assignment_for_rendering(compiler, destination);
            write_many_to_section(
                destination->section,
                "np_float_to_str(",
                assignment_to_convert.variable_name,
                ");\n",
                NULL
            );
            return;
        case PYTYPE_BOOL:
            prepare_c_assignment_for_rendering(compiler, destination);
            write_many_to_section(
                destination->section,
                "np_bool_to_str(",
                assignment_to_convert.variable_name,
                ");\n",
                NULL
            );
            return;
        case PYTYPE_OBJECT:
            convert_object_to_str(compiler, assignment_to_convert, destination);
            break;
        default: {
            static const size_t buflen = 1024;
            char from_type[buflen];
            render_type_info_human_readable(
                assignment_to_convert.type_info, from_type, buflen
            );
            unimplemented_errorf(
                compiler->file_index,
                compiler->current_stmt_location,
                "type conversion from `%s` to `str` not currently implemented",
                from_type
            );
        }
    }
}

static void
render_builtin_print(C_Compiler* compiler, CompilerSection* section, Arguments* args)
{
    if (args->values_count != args->n_positional) {
        type_error(
            compiler->file_index,
            compiler->current_operation_location,
            "print keyword arguments not currently implemented"
        );
    }

    size_t args_count = (args->values_count == 0) ? 1 : args->values_count;
    GENERATE_UNIQUE_VAR_NAMES(compiler, args_count, initial_resolution_vars)
    C_Assignment initial_assignments[args_count];

    if (args->values_count == 0) {
        initial_assignments[0].section = section;
        initial_assignments[0].type_info.type = PYTYPE_STRING;
        initial_assignments[0].variable_name = initial_resolution_vars[0];
        initial_assignments[0].is_declared = false;
        prepare_c_assignment_for_rendering(compiler, initial_assignments);
        write_to_section(section, "{.data=\"\", .length=0};\n");
    }
    else {
        for (size_t i = 0; i < args->values_count; i++) {
            initial_assignments[i].section = section;
            initial_assignments[i].type_info.type = PYTYPE_UNTYPED;
            initial_assignments[i].variable_name = initial_resolution_vars[i];
            initial_assignments[i].is_declared = false;
            render_expression_assignment(
                compiler, initial_assignments + i, args->values[i]
            );
        }
    }

    GENERATE_UNIQUE_VAR_NAMES(compiler, args_count, string_vars)
    C_Assignment converted_assignments[args_count];
    for (size_t i = 0; i < args_count; i++) {
        if (initial_assignments[i].type_info.type == PYTYPE_STRING)
            converted_assignments[i] = initial_assignments[i];
        else {
            converted_assignments[i].type_info.type = PYTYPE_STRING;
            converted_assignments[i].variable_name = string_vars[i];
            converted_assignments[i].is_declared = false;
            converted_assignments[i].section = section;
            convert_assignment_to_string(
                compiler, initial_assignments[i], converted_assignments + i
            );
        }
    }

    char arg_count_as_str[10] = {0};
    sprintf(arg_count_as_str, "%zu", args->values_count);
    write_to_section(section, "builtin_print(");
    write_to_section(section, arg_count_as_str);
    write_to_section(section, ", ");
    for (size_t i = 0; i < args_count; i++) {
        if (i > 0) write_to_section(section, ", ");
        write_to_section(section, converted_assignments[i].variable_name);
    }
    write_to_section(section, ");\n");
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

static void
render_list_append(
    C_Compiler* compiler,
    C_Assignment* assignment,
    C_Assignment* list_assignment,
    Arguments* args
)
{
    TypeInfo return_type = {.type = PYTYPE_NONE};
    set_assignment_type_info(compiler, assignment, return_type);

    TypeInfo list_content_type = list_assignment->type_info.inner->types[0];
    expect_arg_count(compiler, "list.append", args, 1);

    GENERATE_UNIQUE_VAR_NAME(compiler, list_item_var);
    C_Assignment item_assignment = {
        .section = assignment->section,
        .type_info = list_content_type,
        .variable_name = list_item_var,
        .is_declared = false};
    render_expression_assignment(compiler, &item_assignment, args->values[0]);

    write_many_to_section(
        assignment->section,
        "LIST_APPEND(",
        list_assignment->variable_name,
        ", ",
        type_info_to_c_syntax(&compiler->sb, list_content_type),
        ", ",
        list_item_var,
        ");\n",
        NULL
    );
}

static void
render_list_clear(
    C_Compiler* compiler,
    C_Assignment* assignment,
    C_Assignment* list_assignment,
    Arguments* args
)
{
    (void)compiler;
    TypeInfo return_type = {.type = PYTYPE_NONE};
    set_assignment_type_info(compiler, assignment, return_type);

    expect_arg_count(compiler, "list.clear", args, 0);

    write_many_to_section(
        assignment->section, "list_clear(", list_assignment->variable_name, ");\n", NULL
    );
}

static void
render_list_copy(
    C_Compiler* compiler,
    C_Assignment* assignment,
    C_Assignment* list_assignment,
    Arguments* args
)
{
    (void)compiler;
    set_assignment_type_info(compiler, assignment, list_assignment->type_info);

    expect_arg_count(compiler, "list.copy", args, 0);

    prepare_c_assignment_for_rendering(compiler, assignment);
    write_many_to_section(
        assignment->section, "list_copy(", list_assignment->variable_name, ");\n", NULL
    );
}

static void
render_list_count(
    C_Compiler* compiler,
    C_Assignment* assignment,
    C_Assignment* list_assignment,
    Arguments* args
)
{
    TypeInfo list_content_type = list_assignment->type_info.inner->types[0];
    const char* cmp_func = cmp_for_type_info(list_content_type);

    expect_arg_count(compiler, "list.count", args, 1);

    GENERATE_UNIQUE_VAR_NAME(compiler, item_var);
    C_Assignment item_assignment = {
        .section = assignment->section,
        .variable_name = item_var,
        .type_info = list_content_type,
        .is_declared = false};
    render_expression_assignment(compiler, &item_assignment, args->values[0]);

    TypeInfo return_type = {.type = PYTYPE_INT};
    set_assignment_type_info(compiler, assignment, return_type);
    if (!assignment->is_declared) {
        write_type_info_to_section(assignment->section, &compiler->sb, return_type);
        write_to_section(assignment->section, assignment->variable_name);
        write_to_section(assignment->section, ";\n");
    }

    write_many_to_section(
        assignment->section,
        "LIST_COUNT(",
        list_assignment->variable_name,
        ", ",
        type_info_to_c_syntax(&compiler->sb, list_content_type),
        ", ",
        cmp_func,
        ", ",
        item_var,
        ", ",
        assignment->variable_name,
        ");\n",
        NULL
    );
}

static void
render_list_extend(
    C_Compiler* compiler,
    C_Assignment* assignment,
    C_Assignment* list_assignment,
    Arguments* args
)
{
    expect_arg_count(compiler, "list.extend", args, 1);

    GENERATE_UNIQUE_VAR_NAME(compiler, other_var);
    C_Assignment other_assignment = {
        .section = assignment->section,
        .type_info = list_assignment->type_info,
        .variable_name = other_var,
        .is_declared = false};
    render_expression_assignment(compiler, &other_assignment, args->values[0]);

    TypeInfo return_type = {.type = PYTYPE_NONE};
    set_assignment_type_info(compiler, assignment, return_type);

    write_many_to_section(
        assignment->section,
        "list_extend(",
        list_assignment->variable_name,
        ", ",
        other_assignment.variable_name,
        ");\n",
        NULL
    );
}

static void
render_list_index(
    C_Compiler* compiler,
    C_Assignment* assignment,
    C_Assignment* list_assignment,
    Arguments* args
)
{
    // TODO: start/stop
    expect_arg_count(compiler, "list.index", args, 1);

    TypeInfo list_content_type = list_assignment->type_info.inner->types[0];
    const char* cmp = cmp_for_type_info(list_content_type);

    GENERATE_UNIQUE_VAR_NAME(compiler, item_variable);
    C_Assignment item_assignment = {
        .section = assignment->section,
        .type_info = list_content_type,
        .variable_name = item_variable,
        .is_declared = false};
    render_expression_assignment(compiler, &item_assignment, args->values[0]);

    TypeInfo return_type = {.type = PYTYPE_INT};
    set_assignment_type_info(compiler, assignment, return_type);

    if (!assignment->variable_name) {
        // TODO: maybe trying to call this without assigning it is just an error.
        GENERATE_UNIQUE_VAR_NAME(compiler, result_variable);
        assignment->variable_name = result_variable;
        assignment->is_declared = false;
    }

    if (!assignment->is_declared) {
        write_many_to_section(
            assignment->section,
            type_info_to_c_syntax(&compiler->sb, return_type),
            " ",
            assignment->variable_name,
            ";\n",
            NULL
        );
    }

    write_many_to_section(
        assignment->section,
        "LIST_INDEX(",
        list_assignment->variable_name,
        ", ",
        type_info_to_c_syntax(&compiler->sb, list_content_type),
        ", ",
        cmp,
        ", ",
        item_variable,
        ", ",
        assignment->variable_name,
        ");\n",
        NULL
    );
}

static void
render_list_insert(
    C_Compiler* compiler,
    C_Assignment* assignment,
    C_Assignment* list_assignment,
    Arguments* args
)
{
    expect_arg_count(compiler, "list.insert", args, 2);

    GENERATE_UNIQUE_VAR_NAME(compiler, index_variable);
    C_Assignment index_assignment = {
        .section = assignment->section,
        .type_info.type = PYTYPE_INT,
        .variable_name = index_variable,
        .is_declared = false};
    render_expression_assignment(compiler, &index_assignment, args->values[0]);

    TypeInfo list_content_type = list_assignment->type_info.inner->types[0];
    GENERATE_UNIQUE_VAR_NAME(compiler, item_variable);
    C_Assignment item_assignment = {
        .section = assignment->section,
        .type_info = list_content_type,
        .variable_name = item_variable,
        .is_declared = false};
    render_expression_assignment(compiler, &item_assignment, args->values[1]);

    TypeInfo return_type = {.type = PYTYPE_NONE};
    set_assignment_type_info(compiler, assignment, return_type);

    write_many_to_section(
        assignment->section,
        "LIST_INSERT(",
        list_assignment->variable_name,
        ", ",
        type_info_to_c_syntax(&compiler->sb, list_content_type),
        ", ",
        index_variable,
        ", ",
        item_variable,
        ");\n",
        NULL
    );
}

static void
render_list_pop(
    C_Compiler* compiler,
    C_Assignment* assignment,
    C_Assignment* list_assignment,
    Arguments* args
)
{
    GENERATE_UNIQUE_VAR_NAME(compiler, index_variable);
    if (args->values_count == 0) {
        index_variable[0] = '-';
        index_variable[1] = '1';
        index_variable[2] = '\0';
    }
    else if (args->values_count == 1) {
        C_Assignment index_assignment = {
            .section = assignment->section,
            .type_info.type = PYTYPE_INT,
            .variable_name = index_variable,
            .is_declared = false};
        render_expression_assignment(compiler, &index_assignment, args->values[0]);
    }
    else {
        type_errorf(
            compiler->file_index,
            compiler->current_operation_location,
            "list.pop expecting 0-1 arguments but got %zu",
            args->values_count
        );
    }

    TypeInfo list_content_type = list_assignment->type_info.inner->types[0];
    set_assignment_type_info(compiler, assignment, list_content_type);
    if (!assignment->variable_name) {
        GENERATE_UNIQUE_VAR_NAME(compiler, item_variable);
        assignment->variable_name = item_variable;
        assignment->is_declared = false;
    }
    if (!assignment->is_declared) {
        write_many_to_section(
            assignment->section,
            type_info_to_c_syntax(&compiler->sb, list_content_type),
            " ",
            assignment->variable_name,
            ";\n",
            NULL
        );
    }

    write_many_to_section(
        assignment->section,
        "LIST_POP(",
        list_assignment->variable_name,
        ", ",
        type_info_to_c_syntax(&compiler->sb, list_content_type),
        ", ",
        index_variable,
        ", ",
        assignment->variable_name,
        ");\n",
        NULL
    );
}

static void
render_list_remove(
    C_Compiler* compiler,
    C_Assignment* assignment,
    C_Assignment* list_assignment,
    Arguments* args
)
{
    expect_arg_count(compiler, "list.remove", args, 1);

    TypeInfo list_content_type = list_assignment->type_info.inner->types[0];
    const char* cmp = cmp_for_type_info(list_content_type);

    GENERATE_UNIQUE_VAR_NAME(compiler, item_variable);
    C_Assignment item_assignment = {
        .section = assignment->section,
        .type_info = list_content_type,
        .variable_name = item_variable,
        .is_declared = false};
    render_expression_assignment(compiler, &item_assignment, args->values[0]);

    TypeInfo return_type = {.type = PYTYPE_NONE};
    set_assignment_type_info(compiler, assignment, return_type);

    write_many_to_section(
        assignment->section,
        "LIST_REMOVE(",
        list_assignment->variable_name,
        ", ",
        type_info_to_c_syntax(&compiler->sb, list_content_type),
        ", ",
        cmp,
        ", ",
        item_variable,
        ");\n",
        NULL
    );
}

static void
render_list_reverse(
    C_Compiler* compiler,
    C_Assignment* assignment,
    C_Assignment* list_assignment,
    Arguments* args
)
{
    (void)compiler;
    expect_arg_count(compiler, "list.reverse", args, 0);

    TypeInfo return_type = {.type = PYTYPE_NONE};
    set_assignment_type_info(compiler, assignment, return_type);

    write_many_to_section(
        assignment->section, "list_reverse(", list_assignment->variable_name, ");\n", NULL
    );
}

static void
render_list_sort(
    C_Compiler* compiler,
    C_Assignment* assignment,
    C_Assignment* list_assignment,
    Arguments* args
)
{
    // TODO: accept key function argument

    bool bad_args = false;
    if (args->values_count != 0) {
        if (args->n_positional > 0)
            bad_args = true;
        else if (args->values_count > 1)
            bad_args = true;
        else if (strcmp(args->kwds[0], "reverse") != 0)
            bad_args = true;
    }
    if (bad_args) {
        fprintf(
            stderr,
            "ERROR: list.sort expecting either 0 args or boolean keyword arg "
            "`reverse`\n"
        );
        exit(1);
    }

    TypeInfo return_type = {.type = PYTYPE_NONE};
    set_assignment_type_info(compiler, assignment, return_type);

    TypeInfo list_content_type = list_assignment->type_info.inner->types[0];
    const char* rev_cmp = sort_cmp_for_type_info(list_content_type, true);
    const char* norm_cmp = sort_cmp_for_type_info(list_content_type, false);

    GENERATE_UNIQUE_VAR_NAME(compiler, reversed_variable);
    if (args->values_count > 0) {
        C_Assignment reversed_assignment = {
            .section = assignment->section,
            .type_info.type = PYTYPE_BOOL,
            .variable_name = reversed_variable,
            .is_declared = false};
        render_expression_assignment(compiler, &reversed_assignment, args->values[0]);
    }
    else {
        memcpy(reversed_variable, "false", 6);
    }

    write_many_to_section(
        assignment->section,
        "LIST_SORT(",
        list_assignment->variable_name,
        ", ",
        norm_cmp,
        ", ",
        rev_cmp,
        ", ",
        reversed_variable,
        ");\n",
        NULL
    );
}

static void
compile_list_builtin(
    C_Compiler* compiler,
    C_Assignment* assignment,
    C_Assignment* list_assignment,
    char* fn_name,
    Arguments* args
)
{
    assert(fn_name && "fn_name cannot be NULL");
    switch (fn_name[0]) {
        case 'a':
            if (strcmp(fn_name, "append") == 0) {
                render_list_append(compiler, assignment, list_assignment, args);
                return;
            }
            break;
        case 'c':
            if (strcmp(fn_name, "clear") == 0) {
                render_list_clear(compiler, assignment, list_assignment, args);
                return;
            }
            else if (strcmp(fn_name, "copy") == 0) {
                render_list_copy(compiler, assignment, list_assignment, args);
                return;
            }
            else if (strcmp(fn_name, "count") == 0) {
                render_list_count(compiler, assignment, list_assignment, args);
                return;
            }
            break;
        case 'e':
            if (strcmp(fn_name, "extend") == 0) {
                render_list_extend(compiler, assignment, list_assignment, args);
                return;
            }
            break;
        case 'i':
            if (strcmp(fn_name, "index") == 0) {
                render_list_index(compiler, assignment, list_assignment, args);
                return;
            }
            else if (strcmp(fn_name, "insert") == 0) {
                render_list_insert(compiler, assignment, list_assignment, args);
                return;
            }
            break;
        case 'p':
            if (strcmp(fn_name, "pop") == 0) {
                render_list_pop(compiler, assignment, list_assignment, args);
                return;
            }
            break;
        case 'r':
            if (strcmp(fn_name, "remove") == 0) {
                render_list_remove(compiler, assignment, list_assignment, args);
                return;
            }
            else if (strcmp(fn_name, "reverse") == 0) {
                render_list_reverse(compiler, assignment, list_assignment, args);
                return;
            }
            break;
        case 's':
            if (strcmp(fn_name, "sort") == 0) {
                render_list_sort(compiler, assignment, list_assignment, args);
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
render_dict_clear(
    C_Compiler* compiler,
    C_Assignment* assignment,
    C_Assignment* dict_assignment,
    Arguments* args
)
{
    (void)compiler;
    TypeInfo return_type = {.type = PYTYPE_NONE};
    set_assignment_type_info(compiler, assignment, return_type);
    expect_arg_count(compiler, "dict.clear", args, 0);

    write_many_to_section(
        assignment->section, "dict_clear(", dict_assignment->variable_name, ");\n", NULL
    );
}

static void
render_dict_copy(
    C_Compiler* compiler,
    C_Assignment* assignment,
    C_Assignment* dict_assignment,
    Arguments* args
)
{
    (void)compiler;
    expect_arg_count(compiler, "dict.copy", args, 0);

    TypeInfo return_type = dict_assignment->type_info;
    set_assignment_type_info(compiler, assignment, return_type);
    prepare_c_assignment_for_rendering(compiler, assignment);
    write_many_to_section(
        assignment->section, "dict_copy(", dict_assignment->variable_name, ");\n", NULL
    );
}

static void
render_dict_get(
    C_Compiler* compiler,
    C_Assignment* assignment,
    C_Assignment* dict_assignment,
    Arguments* args
)
{
    (void)compiler;
    (void)assignment;
    (void)dict_assignment;
    (void)args;
    UNIMPLEMENTED("dict.get is not implemented");
}

static void
render_dict_items(
    C_Compiler* compiler,
    C_Assignment* assignment,
    C_Assignment* dict_assignment,
    Arguments* args
)
{
    expect_arg_count(compiler, "dict.items", args, 0);

    // ITER of DICT_ITEMS of [KEY_TYPE, VALUE_TYPE]
    TypeInfo dict_items_type = {
        .type = PYTYPE_DICT_ITEMS, .inner = dict_assignment->type_info.inner};
    TypeInfo return_type = {
        .type = PYTYPE_ITER,
        .inner = arena_alloc(compiler->arena, sizeof(TypeInfoInner))};
    return_type.inner->types = arena_alloc(compiler->arena, sizeof(TypeInfo));
    return_type.inner->types[0] = dict_items_type;
    return_type.inner->count = 1;

    set_assignment_type_info(compiler, assignment, return_type);
    prepare_c_assignment_for_rendering(compiler, assignment);
    write_many_to_section(
        assignment->section,
        "dict_iter_items(",
        dict_assignment->variable_name,
        ");\n",
        NULL
    );
}

static void
render_dict_keys(
    C_Compiler* compiler,
    C_Assignment* assignment,
    C_Assignment* dict_assignment,
    Arguments* args
)
{
    expect_arg_count(compiler, "dict.keys", args, 0);

    // ITER of KEY_TYPE
    TypeInfo return_type = {
        .type = PYTYPE_ITER,
        .inner = arena_alloc(compiler->arena, sizeof(TypeInfoInner))};
    return_type.inner->types = arena_alloc(compiler->arena, sizeof(TypeInfo));
    return_type.inner->types[0] = dict_assignment->type_info.inner->types[0];
    return_type.inner->count = 1;

    set_assignment_type_info(compiler, assignment, return_type);
    prepare_c_assignment_for_rendering(compiler, assignment);
    write_many_to_section(
        assignment->section,
        "dict_iter_keys(",
        dict_assignment->variable_name,
        ");\n",
        NULL
    );
}

static void
render_dict_pop(
    C_Compiler* compiler,
    C_Assignment* assignment,
    C_Assignment* dict_assignment,
    Arguments* args
)
{
    TypeInfo return_type = dict_assignment->type_info.inner->types[1];
    set_assignment_type_info(compiler, assignment, return_type);

    // TODO: implement default value
    expect_arg_count(compiler, "dict.pop", args, 1);

    GENERATE_UNIQUE_VAR_NAME(compiler, key_variable);
    C_Assignment key_assignment = {
        .section = assignment->section,
        .type_info = dict_assignment->type_info.inner->types[0],
        .variable_name = key_variable,
        .is_declared = false};
    render_expression_assignment(compiler, &key_assignment, args->values[0]);

    prepare_c_assignment_for_rendering(compiler, assignment);
    write_many_to_section(
        assignment->section,
        "*(",
        type_info_to_c_syntax(&compiler->sb, return_type),
        "*)dict_pop_val(",
        dict_assignment->variable_name,
        ", &",
        key_variable,
        ");\n",
        NULL
    );
}

static void
render_dict_popitem(
    C_Compiler* compiler,
    C_Assignment* assignment,
    C_Assignment* dict_assignment,
    Arguments* args
)
{
    (void)compiler;
    (void)assignment;
    (void)dict_assignment;
    (void)args;
    UNIMPLEMENTED("dict.popitem will not be implemented until tuples are implemented");
}

static void
render_dict_update(
    C_Compiler* compiler,
    C_Assignment* assignment,
    C_Assignment* dict_assignment,
    Arguments* args
)
{
    TypeInfo return_type = {.type = PYTYPE_NONE};
    set_assignment_type_info(compiler, assignment, return_type);

    // TODO: accept args other than another dict
    expect_arg_count(compiler, "dict.update", args, 1);

    GENERATE_UNIQUE_VAR_NAME(compiler, other_dict_variable);
    C_Assignment other_dict_assignment = {
        .section = assignment->section,
        .type_info = dict_assignment->type_info,
        .variable_name = other_dict_variable,
        .is_declared = false};
    render_expression_assignment(compiler, &other_dict_assignment, args->values[0]);

    write_many_to_section(
        assignment->section,
        "dict_update(",
        dict_assignment->variable_name,
        ", ",
        other_dict_variable,
        ");\n",
        NULL
    );
}

static void
render_dict_values(
    C_Compiler* compiler,
    C_Assignment* assignment,
    C_Assignment* dict_assignment,
    Arguments* args
)
{
    expect_arg_count(compiler, "dict.values", args, 0);

    // ITER of KEY_VALUE
    TypeInfo return_type = {
        .type = PYTYPE_ITER,
        .inner = arena_alloc(compiler->arena, sizeof(TypeInfoInner))};
    return_type.inner->types = arena_alloc(compiler->arena, sizeof(TypeInfo));
    return_type.inner->types[0] = dict_assignment->type_info.inner->types[1];
    return_type.inner->count = 1;

    set_assignment_type_info(compiler, assignment, return_type);
    prepare_c_assignment_for_rendering(compiler, assignment);
    write_many_to_section(
        assignment->section,
        "dict_iter_vals(",
        dict_assignment->variable_name,
        ");\n",
        NULL
    );
}

static void
compile_dict_builtin(
    C_Compiler* compiler,
    C_Assignment* assignment,
    C_Assignment* dict_assignment,
    char* fn_name,
    Arguments* args
)
{
    assert(fn_name && "fn_name cannot be NULL");

    switch (fn_name[0]) {
        case 'c':
            if (strcmp(fn_name, "clear") == 0) {
                render_dict_clear(compiler, assignment, dict_assignment, args);
                return;
            }
            else if (strcmp(fn_name, "copy") == 0) {
                render_dict_copy(compiler, assignment, dict_assignment, args);
                return;
            }
            break;
        case 'g':
            if (strcmp(fn_name, "get") == 0) {
                render_dict_get(compiler, assignment, dict_assignment, args);
                return;
            }
            break;
        case 'i':
            if (strcmp(fn_name, "items") == 0) {
                render_dict_items(compiler, assignment, dict_assignment, args);
                return;
            }
            break;
        case 'k':
            if (strcmp(fn_name, "keys") == 0) {
                render_dict_keys(compiler, assignment, dict_assignment, args);
                return;
            }
            break;
        case 'p':
            if (strcmp(fn_name, "pop") == 0) {
                render_dict_pop(compiler, assignment, dict_assignment, args);
                return;
            }
            break;
            if (strcmp(fn_name, "popitem") == 0) {
                render_dict_popitem(compiler, assignment, dict_assignment, args);
                return;
            }
            break;
        case 'u':
            if (strcmp(fn_name, "update") == 0) {
                render_dict_update(compiler, assignment, dict_assignment, args);
                return;
            }
            break;
        case 'v':
            if (strcmp(fn_name, "values") == 0) {
                render_dict_values(compiler, assignment, dict_assignment, args);
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
    C_Compiler* compiler, C_Assignment* assignment, char* fn_identifier, Arguments* args
)
{
    if (strcmp(fn_identifier, "print") == 0) {
        ;
        render_builtin_print(compiler, assignment->section, args);
        return;
    }
    else {
        name_errorf(
            compiler->file_index,
            compiler->current_operation_location,
            "function not implemented: %s",
            fn_identifier
        );
    }
}

static void
require_math_lib(C_Compiler* compiler)
{
    if (compiler->reqs.libs[LIB_MATH]) return;
    compiler->reqs.libs[LIB_MATH] = true;
    write_to_section(&compiler->forward, "#include <math.h>\n");
}

static void
render_object_operation(
    C_Compiler* compiler,
    C_Assignment* assignment,
    Operator op_type,
    char** operand_reprs,
    TypeInfo* types
)
{
    bool is_rop;
    bool is_unary;

    FunctionStatement* fndef =
        find_object_op_function(types[0], types[1], op_type, &is_rop, &is_unary);

    if (!fndef) {
        static const size_t buflen = 1024;
        char left[buflen];
        char right[buflen];
        render_type_info_human_readable(types[0], left, buflen);
        render_type_info_human_readable(types[1], right, buflen);
        type_errorf(
            compiler->file_index,
            compiler->current_operation_location,
            "unsupported operand types for `%s`: `%s` and `%s`",
            op_to_cstr(op_type),
            left,
            right
        );
    }

    set_assignment_type_info(compiler, assignment, fndef->sig.return_type);
    prepare_c_assignment_for_rendering(compiler, assignment);

    if (is_unary) {
        write_many_to_section(
            assignment->section, fndef->ns_ident, "(", operand_reprs[0], ");\n", NULL
        );
    }
    else if (is_rop) {
        write_many_to_section(
            assignment->section,
            fndef->ns_ident,
            "(",
            operand_reprs[1],
            ", ",
            operand_reprs[0],
            ");\n",
            NULL
        );
    }
    else {
        write_many_to_section(
            assignment->section,
            fndef->ns_ident,
            "(",
            operand_reprs[0],
            ", ",
            operand_reprs[1],
            ");\n",
            NULL
        );
    }
}

static void
render_operation(
    C_Compiler* compiler,
    C_Assignment* assignment,
    Operator op_type,
    char** operand_reprs,
    TypeInfo* types
)
{
    if (types[0].type == PYTYPE_OBJECT || types[1].type == PYTYPE_OBJECT) {
        render_object_operation(compiler, assignment, op_type, operand_reprs, types);
        return;
    }

    TypeInfo type_info = resolve_operation_type(types[0], types[1], op_type);
    if (type_info.type == PYTYPE_UNTYPED) {
        static const size_t buflen = 1024;
        char left[buflen];
        char right[buflen];
        render_type_info_human_readable(types[0], left, buflen);
        render_type_info_human_readable(types[1], right, buflen);
        type_errorf(
            compiler->file_index,
            compiler->current_operation_location,
            "unsupported operand types for `%s`: `%s` and `%s`",
            op_to_cstr(op_type),
            left,
            right
        );
    }
    set_assignment_type_info(compiler, assignment, type_info);

    switch (op_type) {
        case OPERATOR_PLUS:
            if (types[0].type == PYTYPE_STRING) {
                prepare_c_assignment_for_rendering(compiler, assignment);
                write_many_to_section(
                    assignment->section,
                    "str_add(",
                    operand_reprs[0],
                    ", ",
                    operand_reprs[1],
                    ");\n",
                    NULL
                );
                return;
            }
            else if (types[0].type == PYTYPE_LIST) {
                prepare_c_assignment_for_rendering(compiler, assignment);
                write_many_to_section(
                    assignment->section,
                    "list_add(",
                    operand_reprs[0],
                    ", ",
                    operand_reprs[1],
                    ");\n",
                    NULL
                );
                return;
            }
            break;
        case OPERATOR_DIV:
            prepare_c_assignment_for_rendering(compiler, assignment);
            write_many_to_section(
                assignment->section,
                (types[0].type == PYTYPE_INT) ? "(PYFLOAT)" : "",
                operand_reprs[0],
                (char*)op_to_cstr(op_type),
                (types[1].type == PYTYPE_INT) ? "(PYFLOAT)" : "",
                operand_reprs[1],
                ";\n",
                NULL
            );
            return;
        case OPERATOR_FLOORDIV:
            prepare_c_assignment_for_rendering(compiler, assignment);
            write_many_to_section(
                assignment->section,
                "(PYINT)(",
                (types[0].type == PYTYPE_INT) ? "(PYFLOAT)" : "",
                operand_reprs[0],
                "/",
                (types[1].type == PYTYPE_INT) ? "(PYFLOAT)" : "",
                operand_reprs[1],
                ");\n",
                NULL
            );
            return;
        case OPERATOR_MOD:
            prepare_c_assignment_for_rendering(compiler, assignment);
            if (assignment->type_info.type == PYTYPE_FLOAT) {
                require_math_lib(compiler);
                write_many_to_section(
                    assignment->section,
                    "fmod(",
                    operand_reprs[0],
                    ", ",
                    operand_reprs[1],
                    ");\n",
                    NULL
                );
            }
            else {
                write_many_to_section(
                    assignment->section,
                    operand_reprs[0],
                    (char*)op_to_cstr(op_type),
                    operand_reprs[1],
                    ";\n",
                    NULL
                );
            }
            return;

        case OPERATOR_POW:
            require_math_lib(compiler);
            prepare_c_assignment_for_rendering(compiler, assignment);
            write_many_to_section(
                assignment->section,
                (assignment->type_info.type == PYTYPE_INT) ? "(PYINT)" : "",
                "pow(",
                operand_reprs[0],
                ", ",
                operand_reprs[1],
                ");\n",
                NULL
            );
            return;
        case OPERATOR_EQUAL:
            if (types[0].type == PYTYPE_STRING) {
                prepare_c_assignment_for_rendering(compiler, assignment);
                write_many_to_section(
                    assignment->section,
                    "str_eq(",
                    operand_reprs[0],
                    ", ",
                    operand_reprs[1],
                    ");\n",
                    NULL
                );
                return;
            }
            break;
        case OPERATOR_GREATER:
            if (types[0].type == PYTYPE_STRING) {
                prepare_c_assignment_for_rendering(compiler, assignment);
                write_many_to_section(
                    assignment->section,
                    "str_gt(",
                    operand_reprs[0],
                    ", ",
                    operand_reprs[1],
                    ");\n",
                    NULL
                );
                return;
            }
            break;
        case OPERATOR_GREATER_EQUAL:
            if (types[0].type == PYTYPE_STRING) {
                prepare_c_assignment_for_rendering(compiler, assignment);
                write_many_to_section(
                    assignment->section,
                    "str_gte(",
                    operand_reprs[0],
                    ", ",
                    operand_reprs[1],
                    ");\n",
                    NULL
                );
                return;
            }
            break;
        case OPERATOR_LESS:
            if (types[0].type == PYTYPE_STRING) {
                prepare_c_assignment_for_rendering(compiler, assignment);
                write_many_to_section(
                    assignment->section,
                    "str_lt(",
                    operand_reprs[0],
                    ", ",
                    operand_reprs[1],
                    ");\n",
                    NULL
                );
                return;
            }
            break;
        case OPERATOR_LESS_EQUAL:
            if (types[0].type == PYTYPE_STRING && types[0].type == PYTYPE_STRING) {
                prepare_c_assignment_for_rendering(compiler, assignment);
                write_many_to_section(
                    assignment->section,
                    "str_lte(",
                    operand_reprs[0],
                    ", ",
                    operand_reprs[1],
                    ");\n",
                    NULL
                );
                return;
            }
            break;
        case OPERATOR_GET_ITEM:
            if (types[0].type == PYTYPE_LIST) {
                if (types[1].type == PYTYPE_SLICE)
                    UNIMPLEMENTED("list slicing unimplemented");
                const char* dest_type_c_syntax =
                    type_info_to_c_syntax(&compiler->sb, types[0].inner->types[0]);
                if (!assignment->is_declared)
                    write_many_to_section(
                        assignment->section,
                        dest_type_c_syntax,
                        " ",
                        assignment->variable_name,
                        ";\n",
                        NULL
                    );
                write_many_to_section(
                    assignment->section,
                    "LIST_GET_ITEM(",
                    operand_reprs[0],
                    ", ",
                    dest_type_c_syntax,
                    ", ",
                    operand_reprs[1],
                    ", ",
                    assignment->variable_name,
                    ");\n",
                    NULL
                );
                return;
            }
            else if (types[0].type == PYTYPE_DICT) {
                prepare_c_assignment_for_rendering(compiler, assignment);
                write_many_to_section(
                    assignment->section,
                    "*(",
                    type_info_to_c_syntax(&compiler->sb, types[0].inner->types[1]),
                    "*)",
                    "dict_get_val(",
                    operand_reprs[0],
                    ", &",
                    operand_reprs[1],
                    ");\n",
                    NULL
                );
                return;
            }
            else {
                UNIMPLEMENTED("getitem unimplemented for this type");
            }
            break;
        default:
            break;
    }
    prepare_c_assignment_for_rendering(compiler, assignment);
    write_many_to_section(
        assignment->section,
        operand_reprs[0],
        (char*)op_to_cstr(op_type),
        operand_reprs[1],
        ";\n",
        NULL
    );
    return;
}

static void
render_operand(C_Compiler* compiler, C_Assignment* assignment, Operand operand)
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
        case OPERAND_EXPRESSION:
            render_expression_assignment(compiler, assignment, operand.expr);
            break;
        case OPERAND_TOKEN:
            render_simple_operand(compiler, assignment, operand);
            break;
        default:
            UNREACHABLE("default case in expression rendering should not be reached");
            break;
    }
}

static void
render_object_method_call(
    C_Compiler* compiler,
    C_Assignment* assignment,
    char* self_identifier,
    FunctionStatement* fndef,
    Arguments* args
)
{
    // patch out self param
    Signature sig = fndef->sig;
    sig.params_count -= 1;
    if (sig.params_count) {
        sig.params++;
        sig.types++;
    }

    // intermediate variables to store args into
    GENERATE_UNIQUE_VAR_NAMES(compiler, sig.params_count, param_vars)
    char* variable_names[sig.params_count];
    for (size_t i = 0; i < sig.params_count; i++) variable_names[i] = param_vars[i];

    render_callable_args_to_variables(
        compiler, assignment->section, args, sig, fndef->name, variable_names, false
    );

    // write the call statement
    set_assignment_type_info(compiler, assignment, fndef->sig.return_type);
    prepare_c_assignment_for_rendering(compiler, assignment);
    write_many_to_section(
        assignment->section, fndef->ns_ident, "(", self_identifier, NULL
    );
    for (size_t arg_i = 0; arg_i < sig.params_count; arg_i++) {
        write_to_section(assignment->section, ", ");
        write_to_section(assignment->section, param_vars[arg_i]);
    }
    write_to_section(assignment->section, ");\n");
}

static void
render_object_creation(
    C_Compiler* compiler,
    C_Assignment* assignment,
    ClassStatement* clsdef,
    Arguments* args
)
{
    // `type` var = np_alloc(bytes);
    set_assignment_type_info(compiler, assignment, clsdef->sig.return_type);
    prepare_c_assignment_for_rendering(compiler, assignment);
    write_many_to_section(
        assignment->section, "np_alloc(sizeof(", clsdef->ns_ident, "));\n", NULL
    );

    FunctionStatement* init = clsdef->object_model_methods[OBJECT_MODEL_INIT];
    if (init) {
        for (size_t i = 0; i < clsdef->sig.defaults_count; i++) {
            C_Assignment default_assignment = {
                .section = assignment->section,
                .type_info = clsdef->sig.types[clsdef->sig.params_count - 1 - i],
                .variable_name = sb_build(
                    &compiler->sb,
                    assignment->variable_name,
                    "->",
                    clsdef->sig.params[clsdef->sig.params_count - 1 - i],
                    NULL
                ),
                .is_declared = true};
            render_expression_assignment(
                compiler,
                &default_assignment,
                clsdef->sig.defaults[clsdef->sig.defaults_count - 1 - i]
            );
        }
        C_Assignment init_assignment = {
            .section = assignment->section, .type_info.type = PYTYPE_NONE};
        render_object_method_call(
            compiler, &init_assignment, assignment->variable_name, init, args
        );
        return;
    }

    // var->param1
    // var->param2
    // ...
    char* value_destinations[clsdef->sig.params_count];
    for (size_t i = 0; i < clsdef->sig.params_count; i++) {
        value_destinations[i] = sb_build(
            &compiler->sb, assignment->variable_name, "->", clsdef->sig.params[i], NULL
        );
    }

    // var->param1 = expr1;
    // var->param2 = expr2;
    // ...
    render_callable_args_to_variables(
        compiler,
        assignment->section,
        args,
        clsdef->sig,
        clsdef->name,
        value_destinations,
        true
    );
}

static void
render_call_operation(
    C_Compiler* compiler, C_Assignment* assignment, Expression* expr, Operation operation
)
{
    char* fn_identifier = expr->operands[operation.left].token.value;
    Arguments* args = expr->operands[operation.right].args;
    Symbol* sym = symbol_hm_get(&compiler->top_level_scope->hm, fn_identifier);
    if (!sym) {
        render_builtin(compiler, assignment, fn_identifier, args);
        return;
    }
    switch (sym->kind) {
        case SYM_FUNCTION: {
            FunctionStatement* fndef = sym->func;
            render_function_call(compiler, assignment, fndef, args);
            break;
        }
        case SYM_CLASS: {
            ClassStatement* clsdef = sym->cls;
            render_object_creation(compiler, assignment, clsdef, args);
            break;
        }
        default:
            type_errorf(
                compiler->file_index,
                *operation.loc,
                "symbol `%s` is not callable",
                fn_identifier
            );
    }
}

static TypeInfo
get_class_member_type_info(
    C_Compiler* compiler, ClassStatement* clsdef, char* member_name
)
{
    for (size_t i = 0; i < clsdef->sig.params_count; i++) {
        if (strcmp(member_name, clsdef->sig.params[i]) == 0) return clsdef->sig.types[i];
    }
    name_errorf(
        compiler->file_index,
        compiler->current_stmt_location,
        "unknown member `%s` for type `%s`",
        member_name,
        clsdef->name
    );
    UNREACHABLE("get member type");
}

static void
render_getattr_operation(
    C_Compiler* compiler,
    C_Assignment* assignment,
    char* left_repr,
    TypeInfo left_type,
    char* attr
)
{
    if (left_type.type != PYTYPE_OBJECT) {
        type_error(
            compiler->file_index,
            compiler->current_operation_location,
            "getattr operation is currently only implemented for `object` types"
        );
    }
    TypeInfo attr_type = get_class_member_type_info(compiler, left_type.cls, attr);
    set_assignment_type_info(compiler, assignment, attr_type);
    prepare_c_assignment_for_rendering(compiler, assignment);
    write_many_to_section(assignment->section, left_repr, "->", attr, ";\n", NULL);
}

static void
render_simple_expression(C_Compiler* compiler, C_Assignment* assignment, Expression* expr)
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
            render_call_operation(compiler, assignment, expr, operation);
            return;
        }
        if (operation.op_type == OPERATOR_GET_ATTR) {
            char* left_repr =
                simple_operand_repr(compiler, expr->operands[operation.left]);
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
        char* operand_reprs[2];
        char variable_memory[2][UNIQUE_VAR_LENGTH] = {0};

        for (size_t lr = (is_unary) ? 1 : 0; lr < 2; lr++) {
            if (operands[lr].kind == OPERAND_TOKEN) {
                operand_reprs[lr] = simple_operand_repr(compiler, operands[lr]);
                operand_types[lr] = resolve_operand_type(&compiler->tc, operands[lr]);
            }
            else {
                WRITE_UNIQUE_VAR_NAME(compiler, variable_memory[lr]);
                C_Assignment operand_assignment = {
                    .section = assignment->section,
                    .variable_name = variable_memory[lr],
                    .is_declared = false};
                render_operand(compiler, &operand_assignment, operands[lr]);
                operand_reprs[lr] = variable_memory[lr];
                operand_types[lr] = operand_assignment.type_info;
            }
        }

        render_operation(
            compiler, assignment, operation.op_type, operand_reprs, operand_types
        );
        return;
    }
    UNREACHABLE("end of write simple expression");
}

typedef struct {
    C_Assignment* assignments;
    C_Assignment** operand_index_to_previous_assignment;
} RenderedOperationMemory;

#define INIT_RENDERED_OPERATION_MEMORY(variable, expression_ptr)                         \
    GENERATE_UNIQUE_VAR_NAMES(                                                           \
        compiler, expr->operations_count - 1, intermediate_variables                     \
    )                                                                                    \
    C_Assignment assignments_memory[expr->operations_count - 1];                         \
    C_Assignment* lookup_memory[expr->operands_count];                                   \
    memset(assignments_memory, 0, sizeof(C_Assignment) * (expr->operations_count - 1));  \
    memset(lookup_memory, 0, sizeof(C_Assignment*) * (expr->operands_count));            \
    for (size_t i = 0; i < expr->operations_count - 1; i++)                              \
        assignments_memory[i].variable_name = intermediate_variables[i];                 \
    variable.assignments = assignments_memory;                                           \
    variable.operand_index_to_previous_assignment = lookup_memory

static void
update_rendered_operation_memory(
    RenderedOperationMemory* mem, C_Assignment* current, Operation operation
)
{
    bool is_unary =
        (operation.op_type == OPERATOR_LOGICAL_NOT ||
         operation.op_type == OPERATOR_NEGATIVE ||
         operation.op_type == OPERATOR_BITWISE_NOT);

    C_Assignment** previously_rendered;

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

static void
render_expression_assignment(
    C_Compiler* compiler, C_Assignment* assignment, Expression* expr
)
{
    if (expr->operations_count <= 1) {
        render_simple_expression(compiler, assignment, expr);
        return;
    }

    RenderedOperationMemory operation_renders;
    INIT_RENDERED_OPERATION_MEMORY(operation_renders, expr);

    for (size_t i = 0; i < expr->operations_count; i++) {
        Operation operation = expr->operations[i];
        compiler->current_operation_location = *operation.loc;
        C_Assignment* this_assignment;

        if (i == expr->operations_count - 1)
            this_assignment = assignment;
        else {
            this_assignment = operation_renders.assignments + i;
            this_assignment->section = assignment->section;
        }

        if (operation.op_type == OPERATOR_CALL) {
            render_call_operation(compiler, this_assignment, expr, operation);
            update_rendered_operation_memory(
                &operation_renders, this_assignment, operation
            );
            continue;
        }
        else if (operation.op_type == OPERATOR_GET_ATTR) {
            C_Assignment left_assignment = {0};
            C_Assignment* previous =
                operation_renders.operand_index_to_previous_assignment[operation.left];
            if (previous)
                left_assignment = *previous;
            else {
                Operand operand = expr->operands[operation.left];
                GENERATE_UNIQUE_VAR_NAME(compiler, left_variable);
                left_assignment.section = assignment->section;
                left_assignment.variable_name = left_variable;
                render_operand(compiler, &left_assignment, operand);
            }
            if (left_assignment.type_info.type == PYTYPE_LIST) {
                if (i == expr->operations_count - 1) {
                    UNIMPLEMENTED("function references are not currently implemented");
                }
                if (++i == expr->operations_count - 1) this_assignment = assignment;
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
                compile_list_builtin(
                    compiler,
                    this_assignment,
                    &left_assignment,
                    expr->operands[operation.right].token.value,
                    expr->operands[next_operation.right].args
                );
                update_rendered_operation_memory(
                    &operation_renders, this_assignment, operation
                );
                update_rendered_operation_memory(
                    &operation_renders, this_assignment, next_operation
                );
                continue;
            }
            else if (left_assignment.type_info.type == PYTYPE_DICT) {
                if (i == expr->operations_count - 1) {
                    UNIMPLEMENTED("function references are not currently implemented");
                }
                if (++i == expr->operations_count - 1) this_assignment = assignment;
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
                    this_assignment,
                    &left_assignment,
                    expr->operands[operation.right].token.value,
                    expr->operands[next_operation.right].args
                );
                update_rendered_operation_memory(
                    &operation_renders, this_assignment, operation
                );
                update_rendered_operation_memory(
                    &operation_renders, this_assignment, next_operation
                );
                continue;
            }
            else if (left_assignment.type_info.type == PYTYPE_OBJECT) {
                render_getattr_operation(
                    compiler,
                    this_assignment,
                    left_assignment.variable_name,
                    left_assignment.type_info,
                    expr->operands[operation.right].token.value
                );
                update_rendered_operation_memory(
                    &operation_renders, this_assignment, operation
                );
                continue;
            }
            else {
                UNIMPLEMENTED(
                    "getattr only currently implemented on lists dicts and objects"
                );
            }
        }

        size_t operand_indices[2] = {operation.left, operation.right};
        TypeInfo operand_types[2] = {0};
        char* operand_reprs[2] = {0};
        char variable_memory[2][UNIQUE_VAR_LENGTH] = {0};

        bool is_unary =
            (operation.op_type == OPERATOR_LOGICAL_NOT ||
             operation.op_type == OPERATOR_NEGATIVE ||
             operation.op_type == OPERATOR_BITWISE_NOT);

        for (size_t lr = (is_unary) ? 1 : 0; lr < 2; lr++) {
            C_Assignment* previously_rendered =
                operation_renders
                    .operand_index_to_previous_assignment[operand_indices[lr]];
            if (previously_rendered) {
                operand_types[lr] = previously_rendered->type_info;
                operand_reprs[lr] = previously_rendered->variable_name;
                continue;
            }

            Operand operand = expr->operands[operand_indices[lr]];
            if (operand.kind == OPERAND_TOKEN) {
                operand_reprs[lr] = simple_operand_repr(compiler, operand);
                operand_types[lr] = resolve_operand_type(&compiler->tc, operand);
            }
            else {
                WRITE_UNIQUE_VAR_NAME(compiler, variable_memory[lr]);
                C_Assignment operand_assignment = {
                    .section = assignment->section,
                    .variable_name = variable_memory[lr],
                    .is_declared = false};
                render_operand(compiler, &operand_assignment, operand);
                operand_reprs[lr] = variable_memory[lr];
                operand_types[lr] = operand_assignment.type_info;
            }
        }

        render_operation(
            compiler, this_assignment, operation.op_type, operand_reprs, operand_types
        );
        update_rendered_operation_memory(&operation_renders, this_assignment, operation);
    }
}

static void
compile_function(C_Compiler* compiler, FunctionStatement* func)
{
    CompilerSection* sections[2] = {
        &compiler->function_declarations, &compiler->function_definitions};

    for (size_t i = 0; i < 2; i++) {
        TypeInfo type_info = func->sig.return_type;

        if (type_info.type == PYTYPE_NONE)
            write_to_section(sections[i], "void ");
        else
            write_type_info_to_section(sections[i], &compiler->sb, type_info);

        write_to_section(sections[i], func->ns_ident);
        write_to_section(sections[i], "(");
        for (size_t j = 0; j < func->sig.params_count; j++) {
            if (j > 0) write_to_section(sections[i], ", ");
            write_type_info_to_section(sections[i], &compiler->sb, func->sig.types[j]);
            write_to_section(sections[i], func->sig.params[j]);
        }
    }
    write_to_section(&compiler->function_declarations, ");\n");
    write_to_section(&compiler->function_definitions, ") {\n");

    // TODO: investigate weather I need a scope stack in the compiler
    // considering the way the type checker is implemented
    LexicalScope* old_locals = compiler->tc.locals;
    compiler->tc.locals = func->scope;
    scope_stack_push(&compiler->scope_stack, func->scope);
    for (size_t i = 0; i < func->body.stmts_count; i++)
        compile_statement(compiler, &compiler->function_definitions, func->body.stmts[i]);
    scope_stack_pop(&compiler->scope_stack);
    compiler->tc.locals = old_locals;

    write_to_section(&compiler->function_definitions, "}\n");
}

static void
compile_class(C_Compiler* compiler, ClassStatement* cls)
{
    if (cls->sig.params_count == 0) {
        unimplemented_error(
            compiler->file_index,
            compiler->current_stmt_location,
            "class defined without any annotated members"
        );
    }
    write_to_section(&compiler->struct_declarations, "typedef struct {");
    for (size_t i = 0; i < cls->sig.params_count; i++) {
        write_type_info_to_section(
            &compiler->struct_declarations, &compiler->sb, cls->sig.types[i]
        );
        write_to_section(&compiler->struct_declarations, cls->sig.params[i]);
        write_to_section(&compiler->struct_declarations, ";");
    }

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
                unimplemented_error(
                    compiler->file_index,
                    stmt->loc,
                    "only function definitions and annotations are currently "
                    "implemented "
                    "within the body of a class"
                );
        }
    }
    write_many_to_section(
        &compiler->struct_declarations, "} ", cls->ns_ident, ";\n", NULL
    );
}

static void
declare_variable(
    C_Compiler* compiler, CompilerSection* section, TypeInfo type, char* identifier
)
{
    write_many_to_section(
        section, type_info_to_c_syntax(&compiler->sb, type), " ", identifier, ";\n", NULL
    );
}

static void
render_list_set_item(
    C_Compiler* compiler,
    C_Assignment list_assignment,
    char* idx_variable,
    char* val_variable
)
{
    write_many_to_section(
        list_assignment.section,
        "LIST_SET_ITEM(",
        list_assignment.variable_name,
        ", ",
        type_info_to_c_syntax(&compiler->sb, list_assignment.type_info.inner->types[0]),
        ", ",
        idx_variable,
        ", ",
        val_variable,
        ");\n",
        NULL
    );
}

static void
render_dict_set_item(C_Assignment dict_assignment, char* key_variable, char* val_variable)
{
    write_many_to_section(
        dict_assignment.section,
        "dict_set_item(",
        dict_assignment.variable_name,
        ", &",
        key_variable,
        ", &",
        val_variable,
        ");\n",
        NULL
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
compile_complex_assignment(
    C_Compiler* compiler, CompilerSection* section, Statement* stmt
)
{
    Operation last_op = stmt->assignment->storage
                            ->operations[stmt->assignment->storage->operations_count - 1];
    Operand last_operand = stmt->assignment->storage->operands[last_op.right];

    // render all but the last operation of left hand side to a variable
    //      ex: a.b.c.d              would render a.b.c to a variable
    //      ex: [1, 2, 3].copy()     would render [1,2,3] to a variable
    Expression container_expr = *stmt->assignment->storage;
    container_expr.operations_count -= 1;
    GENERATE_UNIQUE_VAR_NAME(compiler, container_variable);
    C_Assignment container_assignment = {
        .section = section, .variable_name = container_variable, .is_declared = false};
    render_expression_assignment(compiler, &container_assignment, &container_expr);

    if (last_op.op_type == OPERATOR_GET_ITEM) {
        // set key/val expected types
        TypeInfo key_type_info;
        TypeInfo val_type_info;
        switch (container_assignment.type_info.type) {
            case PYTYPE_LIST: {
                key_type_info.type = PYTYPE_INT;
                val_type_info = container_assignment.type_info.inner->types[0];
                break;
            }
            case PYTYPE_DICT: {
                key_type_info = container_assignment.type_info.inner->types[0];
                val_type_info = container_assignment.type_info.inner->types[1];
                break;
            }
            default:
                UNIMPLEMENTED("setitem not implemented for this data type");
        }

        // render key to a variable
        GENERATE_UNIQUE_VAR_NAME(compiler, key_variable);
        C_Assignment key_assignment = {
            .section = container_assignment.section,
            .type_info = key_type_info,
            .variable_name = key_variable,
            .is_declared = false};
        render_operand(compiler, &key_assignment, last_operand);

        // render value to a variable
        GENERATE_UNIQUE_VAR_NAME(compiler, val_variable);
        if (stmt->assignment->op_type == OPERATOR_ASSIGNMENT) {
            C_Assignment val_assignment = {
                .section = container_assignment.section,
                .type_info = val_type_info,
                .variable_name = val_variable,
                .is_declared = false};
            render_expression_assignment(
                compiler, &val_assignment, stmt->assignment->value
            );
        }
        else {
            // getitem
            GENERATE_UNIQUE_VAR_NAME(compiler, current_val_variable);
            C_Assignment current_val_assignment = {
                .section = container_assignment.section,
                .type_info = val_type_info,
                .variable_name = current_val_variable,
                .is_declared = false};
            char* current_reprs[2] = {
                container_assignment.variable_name, key_assignment.variable_name};
            TypeInfo current_types[2] = {
                container_assignment.type_info, key_assignment.type_info};
            render_operation(
                compiler,
                &current_val_assignment,
                OPERATOR_GET_ITEM,
                current_reprs,
                current_types
            );

            // right side expression
            GENERATE_UNIQUE_VAR_NAME(compiler, other_val_variable);
            C_Assignment other_val_assignment = {
                .section = container_assignment.section,
                .variable_name = other_val_variable,
                .is_declared = false};
            render_expression_assignment(
                compiler, &other_val_assignment, stmt->assignment->value
            );

            // combine current value with right side expression
            Operator op_type = OP_ASSIGNMENT_TO_OP_TABLE[stmt->assignment->op_type];
            C_Assignment val_assignment = {
                .section = container_assignment.section,
                .type_info = val_type_info,
                .variable_name = val_variable,
                .is_declared = false};
            char* new_reprs[2] = {current_val_variable, other_val_variable};
            TypeInfo new_types[2] = {
                current_val_assignment.type_info, other_val_assignment.type_info};
            render_operation(compiler, &val_assignment, op_type, new_reprs, new_types);
        }
        switch (container_assignment.type_info.type) {
            case PYTYPE_LIST: {
                render_list_set_item(
                    compiler, container_assignment, key_variable, val_variable
                );
                return;
            }
            case PYTYPE_DICT: {
                render_dict_set_item(container_assignment, key_variable, val_variable);
                return;
            }
            default:
                UNIMPLEMENTED("setitem not implemented for this data type");
        }
    }
    else if (last_op.op_type == OPERATOR_GET_ATTR) {
        if (container_assignment.type_info.type != PYTYPE_OBJECT) {
            UNIMPLEMENTED("setattr for this type not implemented");
        }
        ClassStatement* clsdef = container_assignment.type_info.cls;
        TypeInfo member_type =
            get_class_member_type_info(compiler, clsdef, last_operand.token.value);
        C_Assignment assignment = {
            .section = container_assignment.section,
            .type_info = member_type,
            .variable_name = sb_build(
                &compiler->sb,
                container_assignment.variable_name,
                "->",
                last_operand.token.value,
                NULL
            ),
            .is_declared = true};

        if (stmt->assignment->op_type == OPERATOR_ASSIGNMENT) {
            // regular assignment
            render_expression_assignment(compiler, &assignment, stmt->assignment->value);
        }
        else {
            // op assignment
            Operator op_type = OP_ASSIGNMENT_TO_OP_TABLE[stmt->assignment->op_type];

            // render getattr
            GENERATE_UNIQUE_VAR_NAME(compiler, current_val_variable);
            C_Assignment current_val_assignment = {
                .section = container_assignment.section,
                .type_info = member_type,
                .variable_name = current_val_variable,

            };
            render_getattr_operation(
                compiler,
                &current_val_assignment,
                container_assignment.variable_name,
                container_assignment.type_info,
                last_operand.token.value
            );

            // render right hand side to variable
            GENERATE_UNIQUE_VAR_NAME(compiler, other_variable);
            C_Assignment other_assignment = {
                .section = container_assignment.section,
                .type_info = member_type,
                .variable_name = other_variable};
            render_expression_assignment(
                compiler, &other_assignment, stmt->assignment->value
            );

            // render operation to combine current value with new expression
            char* reprs[2] = {current_val_variable, other_variable};
            TypeInfo types[2] = {
                current_val_assignment.type_info, other_assignment.type_info};
            render_operation(compiler, &assignment, op_type, reprs, types);
        }
        return;
    }
    else
        UNIMPLEMENTED("complex assignment compilation unimplemented for op type");
}

static void
compile_simple_assignment(C_Compiler* compiler, CompilerSection* section, Statement* stmt)
{
    char* identifier = stmt->assignment->storage->operands[0].token.value;
    Symbol* sym =
        symbol_hm_get(&scope_stack_peek(&compiler->scope_stack)->hm, identifier);

    bool top_level = section == &compiler->init_module_function;
    bool declared;
    char* variable_name;
    char* real_var_name;
    TypeInfo* symtype;

    if (sym->kind == SYM_VARIABLE) {
        symtype = &sym->variable->type;
        variable_name = sym->variable->ns_ident;
        real_var_name = sym->variable->identifier;
        declared = (top_level) ? true : sym->variable->declared;
    }
    else {
        symtype = &sym->semi_scoped->type;
        variable_name = sym->semi_scoped->current_id;
        real_var_name = sym->semi_scoped->identifier;
        declared = true;
    }

    C_Assignment assignment = {
        .section = section,
        .variable_name = variable_name,
        .user_defined_variable_name = real_var_name,
        .type_info = *symtype,
        .is_declared = declared,
        .loc = &stmt->loc,
    };
    render_expression_assignment(compiler, &assignment, stmt->assignment->value);

    if (symtype->type == PYTYPE_UNTYPED) *symtype = assignment.type_info;
    if (top_level && sym->kind == SYM_VARIABLE && !sym->variable->declared)
        declare_variable(
            compiler,
            &compiler->variable_declarations,
            assignment.type_info,
            assignment.variable_name
        );
}

static void
compile_simple_op_assignment(
    C_Compiler* compiler, CompilerSection* section, Statement* stmt
)
{
    char* identifier = stmt->assignment->storage->operands[0].token.value;
    Symbol* sym =
        symbol_hm_get(&scope_stack_peek(&compiler->scope_stack)->hm, identifier);
    Operator op_type = OP_ASSIGNMENT_TO_OP_TABLE[stmt->assignment->op_type];

    char* variable_name;
    char* real_var_name;
    TypeInfo* symtype;

    if (sym->kind == SYM_VARIABLE) {
        symtype = &sym->variable->type;
        variable_name = sym->variable->ns_ident;
        real_var_name = sym->variable->identifier;
    }
    else {
        symtype = &sym->semi_scoped->type;
        variable_name = sym->semi_scoped->current_id;
        real_var_name = sym->semi_scoped->identifier;
    }

    GENERATE_UNIQUE_VAR_NAME(compiler, other_variable);
    C_Assignment other_assignment = {
        .section = section, .variable_name = other_variable, .is_declared = false};
    render_expression_assignment(compiler, &other_assignment, stmt->assignment->value);

    if (symtype->type == PYTYPE_OBJECT) {
        ClassStatement* clsdef = symtype->cls;
        ObjectModel om = op_assignment_to_object_model(stmt->assignment->op_type);
        FunctionStatement* fndef = clsdef->object_model_methods[om];

        if (!fndef) {
            static const size_t buflen = 1024;
            char object_type_hr[buflen];
            render_type_info_human_readable(*symtype, object_type_hr, buflen);
            type_errorf(
                compiler->file_index,
                stmt->loc,
                "type `%s` does not support `%s` operation",
                object_type_hr,
                op_to_cstr(stmt->assignment->op_type)
            );
        }
        if (!compare_types(other_assignment.type_info, fndef->sig.types[1])) {
            static const size_t buflen = 1024;
            char object_type_hr[buflen];
            render_type_info_human_readable(*symtype, object_type_hr, buflen);
            char rvalue_type_hr[buflen];
            render_type_info_human_readable(
                other_assignment.type_info, rvalue_type_hr, buflen
            );
            type_errorf(
                compiler->file_index,
                stmt->loc,
                "type `%s` does not support `%s` operation with `%s` rtype",
                object_type_hr,
                op_to_cstr(stmt->assignment->op_type),
                rvalue_type_hr
            );
        }

        C_Assignment assignment = {
            .section = section,
            .variable_name = variable_name,
            .user_defined_variable_name = real_var_name,
            .type_info = *symtype,
            .is_declared = true,
            .loc = &stmt->loc,
        };
        prepare_c_assignment_for_rendering(compiler, &assignment);
        write_many_to_section(
            section,
            fndef->ns_ident,
            "(",
            variable_name,
            ", ",
            other_assignment.variable_name,
            ");\n",
            NULL
        );
    }
    else {
        C_Assignment assignment = {
            .section = section,
            .variable_name = variable_name,
            .user_defined_variable_name = real_var_name,
            .type_info = *symtype,
            .is_declared = true,
            .loc = &stmt->loc,
        };

        char* operand_reprs[2] = {
            assignment.variable_name, other_assignment.variable_name};
        TypeInfo operand_types[2] = {assignment.type_info, other_assignment.type_info};
        render_operation(compiler, &assignment, op_type, operand_reprs, operand_types);
    }
}

static void
compile_assignment(C_Compiler* compiler, CompilerSection* section, Statement* stmt)
{
    if (stmt->assignment->storage->operations_count != 0)
        compile_complex_assignment(compiler, section, stmt);
    else {
        if (stmt->assignment->op_type != OPERATOR_ASSIGNMENT)
            compile_simple_op_assignment(compiler, section, stmt);
        else
            compile_simple_assignment(compiler, section, stmt);
    }
}

static void
compile_annotation(C_Compiler* compiler, CompilerSection* section, Statement* stmt)
{
    // TODO: implement default values for class members

    LexicalScope* scope = scope_stack_peek(&compiler->scope_stack);
    Symbol* sym = symbol_hm_get(&scope->hm, stmt->annotation->identifier);
    if (sym->kind != SYM_VARIABLE)
        syntax_error(compiler->file_index, stmt->loc, 0, "unexpected annotation");

    if (!sym->variable->declared) {
        CompilerSection* decl_section = (scope == compiler->top_level_scope)
                                            ? &compiler->variable_declarations
                                            : section;
        declare_variable(
            compiler, decl_section, stmt->annotation->type, sym->variable->ns_ident
        );
    }

    if (stmt->annotation->initial) {
        C_Assignment assignment = {
            .loc = &stmt->loc,
            .section = section,
            .type_info = stmt->annotation->type,
            .user_defined_variable_name = sym->variable->identifier,
            .variable_name = sym->variable->ns_ident,
            .is_declared = true};
        render_expression_assignment(compiler, &assignment, stmt->annotation->initial);
    }
}

static void
compile_return_statement(
    C_Compiler* compiler, CompilerSection* section, ReturnStatement* ret
)
{
    GENERATE_UNIQUE_VAR_NAME(compiler, return_var);

    LexicalScope* scope = scope_stack_peek(&compiler->scope_stack);

    C_Assignment assignment = {
        .section = section,
        .type_info = scope->func->sig.return_type,
        .variable_name = return_var,
        .is_declared = false};
    render_expression_assignment(compiler, &assignment, ret->value);

    if (assignment.type_info.type == PYTYPE_NONE) {
        write_to_section(section, "return;\n");
    }
    else {
        write_to_section(section, "return ");
        write_to_section(section, return_var);
        write_to_section(section, ";\n");
    }
}

static char*
init_semi_scoped_variable(
    C_Compiler* compiler, CompilerSection* section, char* identifier, TypeInfo type_info
)
{
    LexicalScope* scope = scope_stack_peek(&compiler->scope_stack);
    Symbol* sym = symbol_hm_get(&scope->hm, identifier);
    if (sym->kind == SYM_VARIABLE) return sym->variable->ns_ident;
    if (!sym->semi_scoped->current_id)
        sym->semi_scoped->current_id = arena_alloc(compiler->arena, UNIQUE_VAR_LENGTH);
    WRITE_UNIQUE_VAR_NAME(compiler, sym->semi_scoped->current_id);
    sym->semi_scoped->type = type_info;
    sym->semi_scoped->directly_in_scope = true;
    write_many_to_section(
        (section == &compiler->init_module_function) ? &compiler->variable_declarations
                                                     : section,
        type_info_to_c_syntax(&compiler->sb, type_info),
        " ",
        sym->semi_scoped->current_id,
        ";\n",
        NULL
    );
    return sym->semi_scoped->current_id;
}

static void
release_semi_scoped_variable(C_Compiler* compiler, char* identifier)
{
    LexicalScope* scope = scope_stack_peek(&compiler->scope_stack);
    Symbol* sym = symbol_hm_get(&scope->hm, identifier);
    sym->semi_scoped->directly_in_scope = false;
}

static void
render_list_for_each_head(
    C_Compiler* compiler,
    CompilerSection* section,
    ForLoopStatement* for_loop,
    C_Assignment iterable
)
{
    if (for_loop->it->identifiers_count > 1 || for_loop->it->identifiers[0].kind != IT_ID)
        UNIMPLEMENTED("for loops with multiple identifiers not currently implemented");

    // TODO: should store these variables on the ItIdentifier struct to avoid lookup
    char* actual_ident = for_loop->it->identifiers[0].name;
    char* it_ident = init_semi_scoped_variable(
        compiler, section, actual_ident, iterable.type_info.inner->types[0]
    );

    // render for loop
    GENERATE_UNIQUE_VAR_NAME(compiler, index_variable);
    write_many_to_section(
        section,
        "LIST_FOR_EACH(",
        iterable.variable_name,
        ", ",
        type_info_to_c_syntax(&compiler->sb, iterable.type_info.inner->types[0]),
        ", ",
        it_ident,
        ", ",
        index_variable,
        ")\n",
        NULL
    );

    release_semi_scoped_variable(compiler, actual_ident);
}

static void
render_dict_for_each_head(
    C_Compiler* compiler,
    CompilerSection* section,
    ForLoopStatement* for_loop,
    C_Assignment iterable
)
{
    if (for_loop->it->identifiers_count > 1 || for_loop->it->identifiers[0].kind != IT_ID)
        UNIMPLEMENTED("for loops with multiple identifiers not currently implemented");

    TypeInfo it_type = iterable.type_info.inner->types[0];
    char* actual_ident = for_loop->it->identifiers[0].name;
    char* it_ident = init_semi_scoped_variable(compiler, section, actual_ident, it_type);

    // render for loop
    GENERATE_UNIQUE_VAR_NAME(compiler, iter_var);
    write_many_to_section(
        section,
        "DICT_ITER_KEYS(",
        iterable.variable_name,
        ", ",
        type_info_to_c_syntax(&compiler->sb, it_type),
        ", ",
        it_ident,
        ", ",
        iter_var,
        ")\n",
        NULL
    );

    release_semi_scoped_variable(compiler, actual_ident);
}

static void
render_dict_items_iterator_for_loop_head(
    C_Compiler* compiler,
    CompilerSection* section,
    ForLoopStatement* for_loop,
    C_Assignment iterable
)
{
    // TODO: once tuples are implemented this distinction can probably be generalized
    // away with the implementation of variable packing/unpacking

    // declare item struct
    GENERATE_UNIQUE_VAR_NAME(compiler, item_struct_variable);
    write_many_to_section(section, "DictItem ", item_struct_variable, ";\n", NULL);

    // parse key value variable names
    char* actual_key_var = NULL;
    char* actual_val_var = NULL;
    if (for_loop->it->identifiers_count == 1 &&
        for_loop->it->identifiers[0].kind == IT_GROUP) {
        ItIdentifier inner_identifier = for_loop->it->identifiers[0];
        if (inner_identifier.group->identifiers_count != 2) goto unexpected_identifiers;
        actual_key_var = inner_identifier.group->identifiers[0].name;
        actual_val_var = inner_identifier.group->identifiers[1].name;
    }
    else if (for_loop->it->identifiers_count == 2) {
        actual_key_var = for_loop->it->identifiers[0].name;
        actual_val_var = for_loop->it->identifiers[1].name;
    }
    else {
    unexpected_identifiers:
        unimplemented_error(
            compiler->file_index,
            compiler->current_stmt_location,
            "expected 2 identifier variable for for dict.items"
        );
    }

    // declare key value variables
    TypeInfoInner* iterable_inner = iterable.type_info.inner;
    TypeInfoInner* items_inner = iterable_inner->types[0].inner;
    TypeInfo key_type = items_inner->types[0];
    TypeInfo val_type = items_inner->types[1];

    // put it variables in scope
    char* key_variable =
        init_semi_scoped_variable(compiler, section, actual_key_var, key_type);
    char* val_variable =
        init_semi_scoped_variable(compiler, section, actual_val_var, val_type);

    // declare void* unpacking variable
    GENERATE_UNIQUE_VAR_NAME(compiler, voidptr_variable);
    write_many_to_section(section, "void* ", voidptr_variable, " = NULL;\n", NULL);

    // render while construct
    // declare its;
    // void* next;
    // while (
    //      (
    //          next = iter.next(),
    //          it = (next) ? unpack next : it,
    //          ...,
    //          next
    //      )
    // )
    // clang-format off
    write_many_to_section(
        section,
        "while ((",
        voidptr_variable," = ",iterable.variable_name,".next(", iterable.variable_name,".iter)", ",\n",
        item_struct_variable," = (",voidptr_variable, ") ? *(DictItem*)",voidptr_variable," : ",item_struct_variable, ",\n",
        key_variable," = (",voidptr_variable,") ? *(",type_info_to_c_syntax(&compiler->sb,key_type),"*)(",item_struct_variable,".key) : ",key_variable,",\n",
        val_variable," = (",voidptr_variable,") ? *(",type_info_to_c_syntax(&compiler->sb,val_type),"*)(",item_struct_variable,".val) : ",val_variable,",\n",
        voidptr_variable,
        "))",
        NULL
    );
    // clang-format on

    release_semi_scoped_variable(compiler, actual_key_var);
    release_semi_scoped_variable(compiler, actual_val_var);
}

static void
render_iterator_for_loop_head(
    C_Compiler* compiler,
    CompilerSection* section,
    ForLoopStatement* for_loop,
    C_Assignment iterable
)
{
    TypeInfoInner* iterable_inner = iterable.type_info.inner;

    if (iterable_inner->count == 1 &&
        iterable_inner->types[0].type == PYTYPE_DICT_ITEMS) {
        render_dict_items_iterator_for_loop_head(compiler, section, for_loop, iterable);
        return;
    }

    if (for_loop->it->identifiers_count > 1 || for_loop->it->identifiers[0].kind != IT_ID)
        UNIMPLEMENTED("for loops with multiple identifiers not currently implemented");

    TypeInfo it_type = iterable.type_info.inner->types[0];
    char* actual_ident = for_loop->it->identifiers[0].name;
    char* it_ident = init_semi_scoped_variable(compiler, section, actual_ident, it_type);

    GENERATE_UNIQUE_VAR_NAME(compiler, voidptr_variable);
    write_many_to_section(section, "void* ", voidptr_variable, ";\n", NULL);

    // declare its;
    // void* next;
    // while (
    //      (
    //          next = iter.next(),
    //          it = (next) ? unpack next : it,
    //          ...,
    //          next
    //      )
    // )
    // clang-format off
    write_many_to_section(
        section,
        "while ((",
        voidptr_variable," = ",iterable.variable_name,".next(", iterable.variable_name,".iter)", ",\n",
        it_ident," = (",voidptr_variable,") ? *(",type_info_to_c_syntax(&compiler->sb, it_type),"*)(",voidptr_variable,") : ",it_ident,",\n",
        voidptr_variable,
        "))",
        NULL
    );
    // clang-format on
    release_semi_scoped_variable(compiler, actual_ident);
}

static void
compile_for_loop(
    C_Compiler* compiler, CompilerSection* section, ForLoopStatement* for_loop
)
{
    GENERATE_UNIQUE_VAR_NAME(compiler, iterable_variable);
    C_Assignment iterable = {
        .section = section, .variable_name = iterable_variable, .is_declared = false};
    render_expression_assignment(compiler, &iterable, for_loop->iterable);

    if (iterable.type_info.type == PYTYPE_LIST)
        render_list_for_each_head(compiler, section, for_loop, iterable);
    else if (iterable.type_info.type == PYTYPE_DICT)
        render_dict_for_each_head(compiler, section, for_loop, iterable);
    else if (iterable.type_info.type == PYTYPE_ITER)
        render_iterator_for_loop_head(compiler, section, for_loop, iterable);
    else {
        fprintf(
            stderr,
            "ERROR: for loops currently implemented only for lists, dicts and "
            "iterators\n"
        );
        exit(1);
    }

    write_to_section(section, "{\n");
    for (size_t i = 0; i < for_loop->body.stmts_count; i++) {
        compile_statement(compiler, section, for_loop->body.stmts[i]);
    }
    write_to_section(section, "}\n");
}

static void
compile_statement(C_Compiler* compiler, CompilerSection* section_or_null, Statement* stmt)
{
    compiler->current_stmt_location = stmt->loc;
    switch (stmt->kind) {
        case STMT_FOR_LOOP: {
            CompilerSection* section =
                (section_or_null) ? section_or_null : &compiler->init_module_function;
            compile_for_loop(compiler, section, stmt->for_loop);
        } break;
        case STMT_IMPORT:
            UNIMPLEMENTED("import compilation is unimplemented");
        case STMT_WHILE:
            UNIMPLEMENTED("while loop compilation is unimplemented");
        case STMT_IF:
            UNIMPLEMENTED("if loop compilation is unimplemented");
        case STMT_TRY:
            UNIMPLEMENTED("try compilation is unimplemented");
        case STMT_WITH:
            UNIMPLEMENTED("with compilation is unimplemented");
        case STMT_CLASS:
            compile_class(compiler, stmt->cls);
            break;
        case STMT_FUNCTION:
            compile_function(compiler, stmt->func);
            break;
        case STMT_ASSIGNMENT: {
            CompilerSection* section =
                (section_or_null) ? section_or_null : &compiler->init_module_function;
            compile_assignment(compiler, section, stmt);
            break;
        }
        case STMT_ANNOTATION: {
            CompilerSection* section =
                (section_or_null) ? section_or_null : &compiler->init_module_function;
            compile_annotation(compiler, section, stmt);
            break;
        }
        case STMT_EXPR: {
            CompilerSection* section =
                (section_or_null) ? section_or_null : &compiler->init_module_function;
            C_Assignment assignment = {
                .section = section, .variable_name = NULL, .is_declared = false};
            render_expression_assignment(compiler, &assignment, stmt->expr);
            break;
        }
        case STMT_NO_OP:
            // might want to put in a `;` but for now just skipping
            break;
        case STMT_EOF:
            break;
        case STMT_RETURN: {
            if (!section_or_null) {
                syntax_error(
                    compiler->file_index,
                    stmt->loc,
                    0,
                    "return statement cannot be defined at the top level scope"
                );
            }
            compile_return_statement(compiler, section_or_null, stmt->ret);
            break;
        }
        case NULL_STMT:
            break;
        default:
            UNREACHABLE("default case unreachable");
    }
}

static void
write_section_to_output(CompilerSection* section, FILE* out)
{
    fwrite(section->buffer, section->capacity - section->remaining, 1, out);
}

static void
write_string_constants_table(C_Compiler* compiler)
{
    write_to_section(
        &compiler->forward, DATATYPE_STRING " " STRING_CONSTANTS_TABLE_NAME "[] = {\n"
    );
    for (size_t i = 0; i < compiler->str_hm.count; i++) {
        StringView sv = compiler->str_hm.elements[i];
        if (i > 0) write_to_section(&compiler->forward, ",\n");
        write_to_section(&compiler->forward, "{.data=\"");
        write_to_section(&compiler->forward, sv.data);
        write_to_section(&compiler->forward, "\", .length=");
        char length_as_str[10];
        sprintf(length_as_str, "%zu", sv.length);
        write_to_section(&compiler->forward, length_as_str);
        write_to_section(&compiler->forward, "}");
    }
    write_to_section(&compiler->forward, "};\n");
}

static void
write_to_output(C_Compiler* compiler, FILE* out)
{
    write_string_constants_table(compiler);
    write_to_section(&compiler->init_module_function, "}\n");
    write_section_to_output(&compiler->forward, out);
    write_section_to_output(&compiler->struct_declarations, out);
    write_section_to_output(&compiler->variable_declarations, out);
    write_section_to_output(&compiler->function_declarations, out);
    write_section_to_output(&compiler->function_definitions, out);
    write_section_to_output(&compiler->init_module_function, out);
    write_section_to_output(&compiler->main_function, out);
    fflush(out);
}

#if DEBUG
static inline void
write_debug_comment_breaks(C_Compiler* compiler)
{
    write_to_section(&compiler->forward, "// FORWARD COMPILER SECTION\n");
    write_to_section(
        &compiler->struct_declarations, "\n// STRUCT DECLARATIONS COMPILER SECTION\n"
    );
    write_to_section(
        &compiler->function_declarations, "\n// FUNCTION DECLARATIONS COMPILER SECTION\n"
    );
    write_to_section(
        &compiler->function_definitions, "\n// FUNCTION DEFINITIONS COMPILER SECTION\n"
    );
    write_to_section(
        &compiler->variable_declarations, "\n// VARIABLE DECLARATIONS COMPILER SECTION\n"
    );
    write_to_section(
        &compiler->init_module_function, "\n// INIT MODULE FUNCTION COMPILER SECTION\n"
    );
    write_to_section(&compiler->main_function, "\n// MAIN FUNCTION COMPILER SECTION\n");
}
#endif

static C_Compiler
compiler_init(Lexer* lexer)
{
    TypeChecker tc = {.arena = lexer->arena, .globals = lexer->top_level, .locals = NULL};
    C_Compiler compiler = {
        .arena = lexer->arena,
        .file_index = lexer->index,
        .top_level_scope = lexer->top_level,
        .tc = tc,
        .sb = sb_init()};
    scope_stack_push(&compiler.scope_stack, lexer->top_level);
#if DEBUG
    write_debug_comment_breaks(&compiler);
#endif
    write_to_section(&compiler.forward, "#include <not_python.h>\n");
    write_to_section(&compiler.init_module_function, "static void init_module(void) {\n");
    write_to_section(&compiler.main_function, "int main(void) {\n");
    write_to_section(&compiler.main_function, "init_module();\n");
    write_to_section(&compiler.main_function, "}");
    return compiler;
}

Requirements
compile_to_c(FILE* outfile, Lexer* lexer)
{
    C_Compiler compiler = compiler_init(lexer);
    for (size_t i = 0; i < lexer->n_statements; i++) {
        compile_statement(&compiler, NULL, lexer->statements[i]);
    }
    write_to_output(&compiler, outfile);

    str_hm_free(&compiler.str_hm);
    sb_free(&compiler.sb);
    section_free(&compiler.forward);
    section_free(&compiler.variable_declarations);
    section_free(&compiler.struct_declarations);
    section_free(&compiler.function_declarations);
    section_free(&compiler.function_definitions);
    section_free(&compiler.init_module_function);
    section_free(&compiler.main_function);
    return compiler.reqs;
}
