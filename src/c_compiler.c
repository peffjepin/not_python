#include "c_compiler.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "np_hash.h"
// TODO: once this module is finished we should find a way to not rely on lexer_helpers
#include "c_compiler_helpers.h"
#include "lexer_helpers.h"
#include "not_python.h"
#include "syntax.h"
#include "type_checker.h"

// TODO: some standard way to implement name mangling will be needed at some point
// TODO: tracking if variables have been initialized before use and warning if not

typedef struct {
    Arena* arena;
    LexicalScope* top_level_scope;
    LexicalScopeStack scope_stack;
    IdentifierSet declared_globals;
    StringHashmap str_hm;
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

#define DATATYPE_INT "PYINT"
#define DATATYPE_FLOAT "PYFLOAT"
#define DATATYPE_STRING "PYSTRING"
#define DATATYPE_BOOL "PYBOOL"
#define DATATYPE_LIST "PYLIST"
#define DATATYPE_DICT "PYDICT"
#define DATATYPE_ITER "PYITER"

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
    char* variable_name;
    bool is_declared;
} C_Assignment;

static void
set_assignment_type_info(C_Assignment* assignment, TypeInfo type_info)
{
    if (assignment->type_info.type != PYTYPE_UNTYPED &&
        !compare_types(type_info, assignment->type_info)) {
        // TODO: error message
        //
        // TODO: some kind of check if this is safe to cast such
        //      ex: if expecting a float, and actually got an int, it's probably safe to
        //      just cast to float
        fprintf(
            stderr, "ERROR: inconsistent typing when assigning type to C_Assignment\n"
        );
        exit(1);
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

static void
untyped_error(void)
{
    // TODO: locations of tokens should be available here to give a reasonable error
    // should it occur
    fprintf(stderr, "encountered untyped variable\n");
    exit(1);
}

static const char*
type_info_to_c_syntax(TypeInfo info)
{
    switch (info.type) {
        case PYTYPE_UNTYPED:
            untyped_error();
            break;
        case PYTYPE_NONE:
            UNIMPLEMENTED("None to c syntax unimplemented");
            break;
        case PYTYPE_INT:
            return DATATYPE_INT;
        case PYTYPE_FLOAT:
            return DATATYPE_FLOAT;
        case PYTYPE_STRING:
            return DATATYPE_STRING;
        case PYTYPE_BOOL:
            return DATATYPE_BOOL;
        case PYTYPE_LIST:
            return DATATYPE_LIST;
        case PYTYPE_TUPLE:
            UNIMPLEMENTED("tuple to c syntax unimplemented");
            break;
        case PYTYPE_DICT:
            return DATATYPE_DICT;
        case PYTYPE_OBJECT:
            UNIMPLEMENTED("object to c syntax unimplemented");
            break;
        case PYTYPE_ITER:
            return DATATYPE_ITER;
        default:
            UNREACHABLE("default type info to c syntax");
            break;
    }
    UNREACHABLE("end of type info to c syntax");
}

static void
write_type_info_to_section(CompilerSection* section, TypeInfo info)
{
    switch (info.type) {
        case PYTYPE_UNTYPED:
            untyped_error();
            break;
        case PYTYPE_NONE:
            UNIMPLEMENTED("None to c syntax unimplemented");
            break;
        case PYTYPE_INT:
            write_to_section(section, DATATYPE_INT " ");
            break;
        case PYTYPE_FLOAT:
            write_to_section(section, DATATYPE_FLOAT " ");
            break;
        case PYTYPE_STRING:
            write_to_section(section, DATATYPE_STRING " ");
            break;
        case PYTYPE_BOOL:
            write_to_section(section, DATATYPE_BOOL " ");
            break;
        case PYTYPE_LIST:
            write_to_section(section, DATATYPE_LIST " ");
            break;
        case PYTYPE_TUPLE:
            UNIMPLEMENTED("tuple to c syntax unimplemented");
            break;
        case PYTYPE_DICT:
            write_to_section(section, DATATYPE_DICT " ");
            break;
        case PYTYPE_OBJECT:
            UNIMPLEMENTED("object to c syntax unimplemented");
            break;
        case PYTYPE_ITER:
            write_to_section(section, DATATYPE_ITER " ");
            break;
        default:
            UNREACHABLE("default type info to c syntax");
            break;
    }
}

static void
prepare_c_assignment_for_rendering(C_Assignment* assignment)
{
    if (assignment->variable_name) {
        if (!assignment->is_declared) {
            if (assignment->type_info.type == PYTYPE_UNTYPED) untyped_error();
            write_type_info_to_section(assignment->section, assignment->type_info);
            assignment->is_declared = true;
        }
        write_to_section(assignment->section, assignment->variable_name);
        write_to_section(assignment->section, " = ");
    }
}

static char*
simple_operand_repr(C_Compiler* compiler, Operand operand)
{
    if (operand.token.type == TOK_IDENTIFIER || operand.token.type == TOK_NUMBER)
        return operand.token.value;
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
    set_assignment_type_info(assignment, resolve_operand_type(&compiler->tc, operand));
    prepare_c_assignment_for_rendering(assignment);
    write_to_section(assignment->section, simple_operand_repr(compiler, operand));
    write_to_section(assignment->section, ";\n");
}

static void
render_empty_enclosure(C_Assignment* enclosure_assignment, Operand operand)
{
    switch (operand.enclosure->type) {
        case ENCLOSURE_LIST:
            prepare_c_assignment_for_rendering(enclosure_assignment);
            write_many_to_section(
                enclosure_assignment->section,
                "LIST_INIT(",
                type_info_to_c_syntax(enclosure_assignment->type_info.inner->types[0]),
                ");\n",
                NULL
            );
            break;
        case ENCLOSURE_DICT:
            prepare_c_assignment_for_rendering(enclosure_assignment);
            TypeInfo key_type = enclosure_assignment->type_info.inner->types[0];
            write_many_to_section(
                enclosure_assignment->section,
                "DICT_INIT(",
                type_info_to_c_syntax(key_type),
                ", ",
                type_info_to_c_syntax(enclosure_assignment->type_info.inner->types[1]),
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
    set_assignment_type_info(enclosure_assignment, enclosure_type_info);

    // TODO: for now we're just going to init an empty list and append everything
    // to it. eventually we should allocate enough room to begin with because we
    // already know the length of the list
    render_empty_enclosure(enclosure_assignment, operand);
    size_t i = 1;
    for (;;) {
        write_many_to_section(
            enclosure_assignment->section,
            "LIST_APPEND(",
            enclosure_assignment->variable_name,
            ", ",
            type_info_to_c_syntax(expression_assignment.type_info),
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
    set_assignment_type_info(enclosure_assignment, enclosure_type_info);

    // TODO: better initialization
    render_empty_enclosure(enclosure_assignment, operand);
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
            // TODO: error message
            fprintf(
                stderr,
                "ERROR: empty containers must have their type annotated when "
                "initialized\n"
            );
            exit(1);
        }
        render_empty_enclosure(enclosure_assignment, operand);
        return;
    }

    if (enclosure_assignment->variable_name == NULL) {
        // TODO: python allows this but I'm not sure it makes sense for us to allow this
        fprintf(stderr, "ERROR: enclosures must be assigned\n");
        exit(1);
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
render_function_args_to_variables(
    C_Compiler* compiler,
    CompilerSection* section,
    Arguments* args,
    Signature sig,
    char** variable_names
)
{
    C_Assignment assignments[sig.params_count];
    memset(assignments, 0, sizeof(C_Assignment) * sig.params_count);

    for (size_t i = 0; i < sig.params_count; i++) {
        assignments[i].section = section;
        assignments[i].type_info = sig.types[i];
        assignments[i].variable_name = variable_names[i];
    }

    if (args->values_count > sig.params_count) {
        // TODO: error message
        fprintf(stderr, "ERROR: too many arguments provided\n");
        exit(1);
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
        int param_i = index_of_kwarg(sig, args->kwds[arg_i - args->n_positional]);
        if (param_i < 0) {
            // TODO: better error message with location diagnostics
            fprintf(stderr, "ERROR: bad keyword argument\n");
            exit(1);
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
            // TODO: better error reporting
            fprintf(stderr, "ERROR: required param not provided\n");
            exit(1);
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
write_variable_to_section_as_type(
    CompilerSection* section, PythonType target_type, char* varname, PythonType vartype
)
{
    if (target_type == vartype) {
        write_to_section(section, varname);
        return;
    }
    switch (target_type) {
        case PYTYPE_STRING:
            switch (vartype) {
                case PYTYPE_INT:
                    write_many_to_section(section, "np_int_to_str(", varname, ")", NULL);
                    break;
                case PYTYPE_FLOAT:
                    write_many_to_section(
                        section, "np_float_to_str(", varname, ")", NULL
                    );
                    break;
                case PYTYPE_BOOL:
                    write_many_to_section(section, "np_bool_to_str(", varname, ")", NULL);
                    break;
                default:
                    // TODO: error message
                    UNIMPLEMENTED("string type conversion unimplemented");
            }
            break;
        default:
            // TODO: error message
            UNIMPLEMENTED("type conversion unimplemented");
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
    // TODO: better error reporting
    if (fndef->sig.return_type.type == PYTYPE_NONE && assignment->variable_name != NULL) {
        fprintf(stderr, "ERROR: trying to assign from a return value of void\n");
        exit(1);
    }

    // intermediate variables to store args into
    GENERATE_UNIQUE_VAR_NAMES(compiler, fndef->sig.params_count, param_vars)
    char* variable_names[fndef->sig.params_count];
    for (size_t i = 0; i < fndef->sig.params_count; i++)
        variable_names[i] = param_vars[i];

    render_function_args_to_variables(
        compiler, assignment->section, args, fndef->sig, variable_names
    );

    // write the call statement
    set_assignment_type_info(assignment, fndef->sig.return_type);
    prepare_c_assignment_for_rendering(assignment);
    write_to_section(assignment->section, fndef->name);
    write_to_section(assignment->section, "(");
    for (size_t arg_i = 0; arg_i < fndef->sig.params_count; arg_i++) {
        if (arg_i > 0) write_to_section(assignment->section, ", ");
        write_to_section(assignment->section, param_vars[arg_i]);
    }
    write_to_section(assignment->section, ");\n");
}

static void
render_builtin_print(C_Compiler* compiler, CompilerSection* section, Arguments* args)
{
    if (args->values_count != args->n_positional) {
        // TODO: error message
        fprintf(stderr, "ERROR: print doesn't take keyword arguments\n");
        exit(1);
    }
    size_t args_count = (args->values_count == 0) ? 1 : args->values_count;
    GENERATE_UNIQUE_VAR_NAMES(compiler, args_count, string_vars)
    TypeInfo var_types[args_count];
    if (args->values_count == 0) {
        TypeInfo default_type_info = {.type = PYTYPE_STRING};
        var_types[0] = default_type_info;
        C_Assignment assignment = {
            .section = section,
            .type_info = default_type_info,
            .variable_name = string_vars[0],
            .is_declared = false};
        prepare_c_assignment_for_rendering(&assignment);
        write_to_section(assignment.section, "{.data=\"\", .length=0};\n");
    }
    else {
        for (size_t i = 0; i < args->values_count; i++) {
            C_Assignment assignment = {
                .section = section,
                .variable_name = string_vars[i],
                .is_declared = false};
            render_expression_assignment(compiler, &assignment, args->values[i]);
            var_types[i] = assignment.type_info;
        }
    }

    char arg_count_as_str[10] = {0};
    sprintf(arg_count_as_str, "%zu", args->values_count);
    write_to_section(section, "builtin_print(");
    write_to_section(section, arg_count_as_str);
    write_to_section(section, ", ");
    for (size_t i = 0; i < args_count; i++) {
        if (i > 0) write_to_section(section, ", ");
        write_variable_to_section_as_type(
            section, PYTYPE_STRING, string_vars[i], var_types[i].type
        );
    }
    write_to_section(section, ");\n");
}

static void
expect_arg_count(Arguments* args, size_t count)
{
    if (args->values_count != count) {
        // TODO: error message
        fprintf(
            stderr,
            "ERROR: list.append expecting %zu argument%s, got %zu\n",
            count,
            (count == 1) ? "" : "s",
            args->values_count
        );
        exit(1);
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
    set_assignment_type_info(assignment, return_type);

    TypeInfo list_content_type = list_assignment->type_info.inner->types[0];
    expect_arg_count(args, 1);

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
        type_info_to_c_syntax(list_content_type),
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
    set_assignment_type_info(assignment, return_type);

    expect_arg_count(args, 0);

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
    set_assignment_type_info(assignment, list_assignment->type_info);

    expect_arg_count(args, 0);

    prepare_c_assignment_for_rendering(assignment);
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

    expect_arg_count(args, 1);

    GENERATE_UNIQUE_VAR_NAME(compiler, item_var);
    C_Assignment item_assignment = {
        .section = assignment->section,
        .variable_name = item_var,
        .type_info = list_content_type,
        .is_declared = false};
    render_expression_assignment(compiler, &item_assignment, args->values[0]);

    TypeInfo return_type = {.type = PYTYPE_INT};
    set_assignment_type_info(assignment, return_type);
    if (!assignment->is_declared) {
        write_type_info_to_section(assignment->section, return_type);
        write_to_section(assignment->section, assignment->variable_name);
        write_to_section(assignment->section, ";\n");
    }

    write_many_to_section(
        assignment->section,
        "LIST_COUNT(",
        list_assignment->variable_name,
        ", ",
        type_info_to_c_syntax(list_content_type),
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
    expect_arg_count(args, 1);

    GENERATE_UNIQUE_VAR_NAME(compiler, other_var);
    C_Assignment other_assignment = {
        .section = assignment->section,
        .type_info = list_assignment->type_info,
        .variable_name = other_var,
        .is_declared = false};
    render_expression_assignment(compiler, &other_assignment, args->values[0]);

    TypeInfo return_type = {.type = PYTYPE_NONE};
    set_assignment_type_info(assignment, return_type);

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
    expect_arg_count(args, 1);

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
    set_assignment_type_info(assignment, return_type);

    if (!assignment->variable_name) {
        // TODO: maybe trying to call this without assigning it is just an error.
        GENERATE_UNIQUE_VAR_NAME(compiler, result_variable);
        assignment->variable_name = result_variable;
        assignment->is_declared = false;
    }

    if (!assignment->is_declared) {
        write_many_to_section(
            assignment->section,
            type_info_to_c_syntax(return_type),
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
        type_info_to_c_syntax(list_content_type),
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
    expect_arg_count(args, 2);

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
    set_assignment_type_info(assignment, return_type);

    write_many_to_section(
        assignment->section,
        "LIST_INSERT(",
        list_assignment->variable_name,
        ", ",
        type_info_to_c_syntax(list_content_type),
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
        // TODO: error message
        fprintf(
            stderr,
            "ERROR: list.pop expecting 0-1 arguments, got %zu\n",
            args->values_count
        );
        exit(1);
    }

    TypeInfo list_content_type = list_assignment->type_info.inner->types[0];
    set_assignment_type_info(assignment, list_content_type);
    if (!assignment->variable_name) {
        GENERATE_UNIQUE_VAR_NAME(compiler, item_variable);
        assignment->variable_name = item_variable;
        assignment->is_declared = false;
    }
    if (!assignment->is_declared) {
        write_many_to_section(
            assignment->section,
            type_info_to_c_syntax(list_content_type),
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
        type_info_to_c_syntax(list_content_type),
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
    expect_arg_count(args, 1);

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
    set_assignment_type_info(assignment, return_type);

    write_many_to_section(
        assignment->section,
        "LIST_REMOVE(",
        list_assignment->variable_name,
        ", ",
        type_info_to_c_syntax(list_content_type),
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
    expect_arg_count(args, 0);

    TypeInfo return_type = {.type = PYTYPE_NONE};
    set_assignment_type_info(assignment, return_type);

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
    set_assignment_type_info(assignment, return_type);

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
            // TODO: error message
            fprintf(stderr, "ERROR: unrecognized list builtin -> %s\n", fn_name);
            exit(1);
            break;
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
    UNIMPLEMENTED("dict.clear is not implemented");
}

static void
render_dict_copy(
    C_Compiler* compiler,
    C_Assignment* assignment,
    C_Assignment* dict_assignment,
    Arguments* args
)
{
    UNIMPLEMENTED("dict.copy is not implemented");
}

static void
render_dict_get(
    C_Compiler* compiler,
    C_Assignment* assignment,
    C_Assignment* dict_assignment,
    Arguments* args
)
{
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
    // ITER of DICT_ITEMS of [KEY_TYPE, VALUE_TYPE]
    TypeInfo dict_items_type = {
        .type = PYTYPE_DICT_ITEMS, .inner = dict_assignment->type_info.inner};
    TypeInfo return_type = {
        .type = PYTYPE_ITER,
        .inner = arena_alloc(compiler->arena, sizeof(TypeInfoInner))};
    return_type.inner->types = arena_alloc(compiler->arena, sizeof(TypeInfo));
    return_type.inner->types[0] = dict_items_type;
    return_type.inner->count = 1;

    set_assignment_type_info(assignment, return_type);
    prepare_c_assignment_for_rendering(assignment);
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
    UNIMPLEMENTED("dict.keys is not implemented");
}

static void
render_dict_pop(
    C_Compiler* compiler,
    C_Assignment* assignment,
    C_Assignment* dict_assignment,
    Arguments* args
)
{
    UNIMPLEMENTED("dict.pop is not implemented");
}

static void
render_dict_popitem(
    C_Compiler* compiler,
    C_Assignment* assignment,
    C_Assignment* dict_assignment,
    Arguments* args
)
{
    UNIMPLEMENTED("dict.popitem is not implemented");
}

static void
render_dict_update(
    C_Compiler* compiler,
    C_Assignment* assignment,
    C_Assignment* dict_assignment,
    Arguments* args
)
{
    UNIMPLEMENTED("dict.update is not implemented");
}

static void
render_dict_values(
    C_Compiler* compiler,
    C_Assignment* assignment,
    C_Assignment* dict_assignment,
    Arguments* args
)
{
    UNIMPLEMENTED("dict.values is not implemented");
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
            // TODO: error message
            fprintf(stderr, "ERROR: unrecognized dict builtin -> %s\n", fn_name);
            exit(1);
            break;
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
        // TODO: better error message
        fprintf(stderr, "ERROR: function undefined (%s)\n", fn_identifier);
        exit(1);
    }
}

static void
render_operation(
    C_Assignment* assignment, Operator op_type, char** operand_reprs, TypeInfo* types
)
{
    TypeInfo type_info = resolve_operation_type(types[0], types[1], op_type);
    if (type_info.type == PYTYPE_UNTYPED) {
        fprintf(stderr, "ERROR: failed to resolve operand type\n");
        exit(1);
    }
    set_assignment_type_info(assignment, type_info);

    switch (op_type) {
        case OPERATOR_PLUS:
            if (types[0].type == PYTYPE_STRING) {
                prepare_c_assignment_for_rendering(assignment);
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
                prepare_c_assignment_for_rendering(assignment);
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
            prepare_c_assignment_for_rendering(assignment);
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
        case OPERATOR_EQUAL:
            if (types[0].type == PYTYPE_STRING) {
                prepare_c_assignment_for_rendering(assignment);
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
                prepare_c_assignment_for_rendering(assignment);
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
                prepare_c_assignment_for_rendering(assignment);
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
                prepare_c_assignment_for_rendering(assignment);
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
                prepare_c_assignment_for_rendering(assignment);
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
                    type_info_to_c_syntax(types[0].inner->types[0]);
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
                prepare_c_assignment_for_rendering(assignment);
                write_many_to_section(
                    assignment->section,
                    "*(",
                    type_info_to_c_syntax(types[0].inner->types[1]),
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
    prepare_c_assignment_for_rendering(assignment);
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
    FunctionStatement* fndef = sym->func;
    render_function_call(compiler, assignment, fndef, args);
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

        if (operation.op_type == OPERATOR_CALL) {
            render_call_operation(compiler, assignment, expr, operation);
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

        render_operation(assignment, operation.op_type, operand_reprs, operand_types);
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
                    // TODO: error message
                    fprintf(stderr, "ERROR: expecting function call\n");
                    exit(1);
                }
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
                    // TODO: error message
                    fprintf(stderr, "ERROR: expecting function call\n");
                    exit(1);
                }
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
            else {
                UNIMPLEMENTED("getattr only currently implemented on lists and dicts");
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
            this_assignment, operation.op_type, operand_reprs, operand_types
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

        // TODO: if we implemented functions returning None as void
        // then it will have to be an error to explicitly `return None`
        // unless the return type is a Union including None or Optional
        // both of which are unimplemented at the time of writing.
        if (type_info.type == PYTYPE_NONE)
            write_to_section(sections[i], "void ");
        else
            write_type_info_to_section(sections[i], type_info);

        write_to_section(sections[i], func->name);
        write_to_section(sections[i], "(");
        for (size_t j = 0; j < func->sig.params_count; j++) {
            if (j > 0) write_to_section(sections[i], ", ");
            write_type_info_to_section(sections[i], func->sig.types[j]);
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
compile_method(C_Compiler* compiler, ClassStatement* cls, FunctionStatement* func)
{
    // TODO: compile function with extra name mangling from the class
    (void)cls;
    compile_function(compiler, func);
}

static void
compile_class(C_Compiler* compiler, ClassStatement* cls)
{
    write_to_section(&compiler->struct_declarations, "struct ");
    // TODO: name mangling
    write_to_section(&compiler->struct_declarations, cls->name);
    write_to_section(&compiler->struct_declarations, "{");
    size_t member_count = 0;
    for (size_t i = 0; i < cls->scope->hm.elements_count; i++) {
        Symbol sym = cls->scope->hm.elements[i];
        switch (sym.kind) {
            case SYM_FUNCTION:
                compile_method(compiler, cls, sym.func);
                break;
            case SYM_VARIABLE:
                // TODO: incomplete -- type objects don't currently exist
                break;
            case SYM_MEMBER:
                if (member_count > 0)
                    write_to_section(&compiler->struct_declarations, ", ");
                write_type_info_to_section(
                    &compiler->struct_declarations, sym.member->type
                );
                write_to_section(&compiler->struct_declarations, sym.member->identifier);
                member_count += 1;
                break;
            case SYM_CLASS:
                UNREACHABLE("nested classes should error in the lexer");
            default:
                UNREACHABLE("forward_declare_class default case unreachable")
        }
    }
    write_to_section(&compiler->struct_declarations, "};\n");
    if (member_count == 0) {
        // TODO: better error reporting or perhaps this should be an error from the
        // parser also worth considering if classes can exist without members as
        // namespaces for functions
        fprintf(stderr, "ERROR: class defined with no members\n");
        exit(1);
    }
}

static void
inconsistent_typing(void)
{
    // TODO: provide context and a good error message
    fprintf(stderr, "ERROR: inconsistent typing");
    exit(1);
}

static void
declare_global_variable(C_Compiler* compiler, TypeInfo type, char* identifier)
{
    if (!id_set_add(&compiler->declared_globals, identifier)) {
        // already declared
        return;
    }
    Symbol* sym = symbol_hm_get(&compiler->top_level_scope->hm, identifier);
    if (sym->variable->type.type == PYTYPE_UNTYPED) {
        sym->variable->type = type;
    }
    else if (!compare_types(type, sym->variable->type)) {
        inconsistent_typing();
    }
    write_type_info_to_section(&compiler->variable_declarations, type);
    write_to_section(&compiler->variable_declarations, identifier);
    write_to_section(&compiler->variable_declarations, ";\n");
}

static void
render_list_set_item(
    C_Compiler* compiler, C_Assignment list_assignment, AssignmentStatement* stmt
)
{
    Operand last_operand =
        stmt->storage->operands
            [stmt->storage->operations[stmt->storage->operations_count - 1].right];

    if (last_operand.kind == OPERAND_SLICE)
        UNIMPLEMENTED("list setitem from slice is unimplemented");
    // render index to a variable
    GENERATE_UNIQUE_VAR_NAME(compiler, index_var);
    C_Assignment index_assignment = {
        .section = list_assignment.section,
        .type_info.type = PYTYPE_INT,
        .variable_name = index_var,
        .is_declared = false};
    if (last_operand.kind == OPERAND_TOKEN)
        render_simple_operand(compiler, &index_assignment, last_operand);
    else if (last_operand.kind == OPERAND_EXPRESSION)
        render_expression_assignment(compiler, &index_assignment, last_operand.expr);
    else
        UNREACHABLE("unexpected operand kind for setitem index")

    // render item to a variable
    GENERATE_UNIQUE_VAR_NAME(compiler, item_var);
    C_Assignment item_assignment = {
        .section = list_assignment.section,
        .type_info = list_assignment.type_info.inner->types[0],
        .variable_name = item_var,
        .is_declared = false};
    render_expression_assignment(compiler, &item_assignment, stmt->value);

    // render setitem
    write_many_to_section(
        list_assignment.section,
        "LIST_SET_ITEM(",
        list_assignment.variable_name,
        ", ",
        type_info_to_c_syntax(list_assignment.type_info.inner->types[0]),
        ", ",
        index_var,
        ", ",
        item_var,
        ");\n",
        NULL
    );
}

static void
render_dict_set_item(
    C_Compiler* compiler, C_Assignment dict_assignment, AssignmentStatement* stmt
)
{
    Operand last_operand =
        stmt->storage->operands
            [stmt->storage->operations[stmt->storage->operations_count - 1].right];

    TypeInfo key_type_info = dict_assignment.type_info.inner->types[0];
    TypeInfo val_type_info = dict_assignment.type_info.inner->types[1];

    // render key to a variable
    GENERATE_UNIQUE_VAR_NAME(compiler, key_variable);
    C_Assignment key_assignment = {
        .section = dict_assignment.section,
        .type_info = key_type_info,
        .variable_name = key_variable,
        .is_declared = false};
    if (last_operand.kind == OPERAND_TOKEN)
        render_simple_operand(compiler, &key_assignment, last_operand);
    else if (last_operand.kind == OPERAND_EXPRESSION)
        render_expression_assignment(compiler, &key_assignment, last_operand.expr);
    else
        UNREACHABLE("unexpected operand kind for dict setitem key")

    // render val to a variable
    GENERATE_UNIQUE_VAR_NAME(compiler, val_variable);
    C_Assignment val_assignment = {
        .section = dict_assignment.section,
        .type_info = val_type_info,
        .variable_name = val_variable,
        .is_declared = false};
    render_expression_assignment(compiler, &val_assignment, stmt->value);

    // render set item operation
    write_many_to_section(
        dict_assignment.section,
        "dict_set_item(",
        dict_assignment.variable_name,
        ", &",
        key_assignment.variable_name,
        ", &",
        val_assignment.variable_name,
        ");\n",
        NULL
    );
}

static void
compile_complex_assignment(
    C_Compiler* compiler, CompilerSection* section, AssignmentStatement* stmt
)
{
    Operation last_op = stmt->storage->operations[stmt->storage->operations_count - 1];

    if (last_op.op_type == OPERATOR_GET_ITEM) {
        Expression container_expr = *stmt->storage;
        container_expr.operations_count -= 1;
        GENERATE_UNIQUE_VAR_NAME(compiler, container_variable);
        C_Assignment container_assignment = {
            .section = section,
            .variable_name = container_variable,
            .is_declared = false};
        render_expression_assignment(compiler, &container_assignment, &container_expr);
        switch (container_assignment.type_info.type) {
            case PYTYPE_LIST: {
                render_list_set_item(compiler, container_assignment, stmt);
                break;
            }
            case PYTYPE_DICT:
                render_dict_set_item(compiler, container_assignment, stmt);
                break;
            default:
                UNIMPLEMENTED("setitem not implemented for this data type");
        }
    }
    else
        UNIMPLEMENTED("complex assignment compilation unimplemented");
}

static void
compile_simple_assignment(
    C_Compiler* compiler, CompilerSection* section, AssignmentStatement* stmt
)
{
    C_Assignment assignment = {
        .section = section,
        .variable_name = stmt->storage->operands[0].token.value,
        .is_declared = true};
    render_expression_assignment(compiler, &assignment, stmt->value);
    if (section == &compiler->init_module_function)
        declare_global_variable(compiler, assignment.type_info, assignment.variable_name);
}

// TODO: looks like this always assumes it's dealing with a global variable which is
// not the case
static void
compile_assignment(
    C_Compiler* compiler, CompilerSection* section, AssignmentStatement* stmt
)
{
    if (stmt->op_type != OPERATOR_ASSIGNMENT)
        UNIMPLEMENTED("assignment op_type not implemented");
    if (stmt->storage->operations_count != 0)
        compile_complex_assignment(compiler, section, stmt);
    else
        compile_simple_assignment(compiler, section, stmt);
}

static void
compile_annotation(
    C_Compiler* compiler, CompilerSection* section, AnnotationStatement* annotation
)
{
    // TODO: implement default values for class members

    LexicalScope* scope = scope_stack_peek(&compiler->scope_stack);

    if (scope == compiler->top_level_scope)
        declare_global_variable(compiler, annotation->type, annotation->identifier);

    if (annotation->initial) {
        // TODO: is_declared probably == false when not at top level scope
        C_Assignment assignment = {
            .section = section,
            .type_info = annotation->type,
            .variable_name = annotation->identifier,
            .is_declared = true};
        render_expression_assignment(compiler, &assignment, annotation->initial);
    }
}

static void
compile_return_statement(
    C_Compiler* compiler, CompilerSection* section, ReturnStatement* ret
)
{
    GENERATE_UNIQUE_VAR_NAME(compiler, return_var);
    C_Assignment assignment = {
        .section = section, .variable_name = return_var, .is_declared = false};
    render_expression_assignment(compiler, &assignment, ret->value);
    write_to_section(section, "return ");
    write_to_section(section, return_var);
    write_to_section(section, ";\n");
}

static void
put_variable_into_current_scope(C_Compiler* compiler, char* name, TypeInfo type_info)
{
    LexicalScope* scope = scope_stack_peek(&compiler->scope_stack);
    Symbol sym = {
        .kind = SYM_VARIABLE, .variable = arena_alloc(compiler->arena, sizeof(Variable))};
    sym.variable->identifier = name;
    sym.variable->type = type_info;
    symbol_hm_put(&scope->hm, sym);
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

    // put it variables into scope with type info
    put_variable_into_current_scope(
        compiler, for_loop->it->identifiers[0].name, iterable.type_info.inner->types[0]
    );

    // render for loop
    GENERATE_UNIQUE_VAR_NAME(compiler, index_variable);
    write_many_to_section(
        section,
        "LIST_FOR_EACH(",
        iterable.variable_name,
        ", ",
        type_info_to_c_syntax(iterable.type_info.inner->types[0]),
        ", ",
        for_loop->it->identifiers[0].name,
        ", ",
        index_variable,
        ")\n",
        NULL
    );
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

    // TODO: at some point some decistion will need made and enforced about the
    // lifetime of these variables. In python you could reuse common iterable
    // identifiers because types can change. It would probably produce a funky error
    // were used again inappropriately. Ultimately either the parser or the compiler
    // should solely handle populating this data structure.
    TypeInfo it_type = iterable.type_info.inner->types[0];
    char* it_ident = for_loop->it->identifiers[0].name;
    put_variable_into_current_scope(compiler, it_ident, it_type);

    // render for loop
    GENERATE_UNIQUE_VAR_NAME(compiler, iter_var);
    write_many_to_section(
        section,
        "DICT_ITER_KEYS(",
        iterable.variable_name,
        ", ",
        type_info_to_c_syntax(it_type),
        ", ",
        it_ident,
        ", ",
        iter_var,
        ")\n",
        NULL
    );
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
    char* key_variable;
    char* val_variable;
    if (for_loop->it->identifiers_count == 1 &&
        for_loop->it->identifiers[0].kind == IT_GROUP) {
        ItIdentifier inner_identifier = for_loop->it->identifiers[0];
        if (inner_identifier.group->identifiers_count != 2) goto unexpected_identifiers;
        key_variable = inner_identifier.group->identifiers[0].name;
        val_variable = inner_identifier.group->identifiers[1].name;
    }
    else if (for_loop->it->identifiers_count == 2) {
        key_variable = for_loop->it->identifiers[0].name;
        val_variable = for_loop->it->identifiers[1].name;
    }
    else {
    unexpected_identifiers:
        // TODO: error message
        fprintf(stderr, "ERROR: unexpected amount of values to unpack\n");
        exit(1);
    }

    // declare key value variables
    TypeInfoInner* iterable_inner = iterable.type_info.inner;
    TypeInfoInner* items_inner = iterable_inner->types[0].inner;
    TypeInfo key_type = items_inner->types[0];
    TypeInfo val_type = items_inner->types[1];
    write_many_to_section(
        section,
        type_info_to_c_syntax(key_type),
        " ",
        key_variable,
        ";\n",
        type_info_to_c_syntax(val_type),
        " ",
        val_variable,
        ";\n",
        NULL
    );

    // put it variables in scope
    // TODO: (maybe should be done by parser)
    put_variable_into_current_scope(compiler, key_variable, key_type);
    put_variable_into_current_scope(compiler, val_variable, val_type);

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
        key_variable," = (",voidptr_variable,") ? *(",type_info_to_c_syntax(key_type),"*)(",item_struct_variable,".key) : ",key_variable,",\n",
        val_variable," = (",voidptr_variable,") ? *(",type_info_to_c_syntax(val_type),"*)(",item_struct_variable,".val) : ",val_variable,",\n",
        voidptr_variable,
        "))",
        NULL
    );
    // clang-format on
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
    UNIMPLEMENTED("standard iterator iteration not yet implemented");
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
            "ERROR: for loops currently implemented only for lists, dicts and iterators\n"
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
            compile_assignment(compiler, section, stmt->assignment);
            break;
        }
        case STMT_ANNOTATION: {
            CompilerSection* section =
                (section_or_null) ? section_or_null : &compiler->init_module_function;
            compile_annotation(compiler, section, stmt->annotation);
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
                // TODO: error message
                fprintf(stderr, "ERROR: return statement can't be in top level scope\n");
                exit(1);
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

void
compile_to_c(FILE* outfile, Lexer* lexer)
{
    C_Compiler compiler = compiler_init(lexer);
    for (size_t i = 0; i < lexer->n_statements; i++) {
        compile_statement(&compiler, NULL, lexer->statements[i]);
    }
    write_to_output(&compiler, outfile);

    str_hm_free(&compiler.str_hm);
    id_set_free(&compiler.declared_globals);
    sb_free(&compiler.sb);
    section_free(&compiler.forward);
    section_free(&compiler.variable_declarations);
    section_free(&compiler.struct_declarations);
    section_free(&compiler.function_declarations);
    section_free(&compiler.function_definitions);
    section_free(&compiler.init_module_function);
    section_free(&compiler.main_function);
}
