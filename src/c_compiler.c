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
    CompilerSection* current_variable_declaration_section;
    size_t unique_vars_counter;
    size_t goto_counter;
    StringBuilder sb;
} C_Compiler;

#define GENERATED_VARIABLE_CAPACITY 16

typedef struct {
    TypeInfo typing;
    bool declared;
    char* source_name;
    char* final_name;
    char local_storage[GENERATED_VARIABLE_CAPACITY];
} C_Variable;

typedef struct {
    CompilerSection* section;
    C_Variable variable;
    Location* loc;  // NULL for intermediate assignments
} C_Assignment;

static void compile_statement(
    C_Compiler* compiler, CompilerSection* section, Statement* stmt
);

static void render_expression_assignment(
    C_Compiler* compiler, C_Assignment* assignment, Expression* expr
);

static void convert_assignment_to_string(
    C_Compiler* compiler, C_Assignment assignment_to_convert, C_Assignment* destination
);

static void declare_variable(C_Compiler* compiler, TypeInfo type, char* identifier);

// TODO: unique vars should probably be namespaced by module in the future

#define GENERATE_UNIQUE_VAR_NAME(compiler_ptr, variable)                                 \
    do {                                                                                 \
        sprintf(                                                                         \
            variable.local_storage, "NP_var%zu", compiler_ptr->unique_vars_counter++     \
        );                                                                               \
        variable.final_name = variable.local_storage;                                    \
        variable.declared = false;                                                       \
    } while (0)

#define GENERATE_UNIQUE_GOTO_IDENTIFIER(compiler_ptr, variable_name)                     \
    char variable_name[GENERATED_VARIABLE_CAPACITY];                                     \
    sprintf(variable_name, "goto%zu", compiler_ptr->goto_counter++)

#define WRITE_UNIQUE_VAR_NAME(compiler_ptr, dest)                                        \
    sprintf(dest, "NP_var%zu", compiler_ptr->unique_vars_counter++)

#define STRING_CONSTANTS_TABLE_NAME "NOT_PYTHON_STRING_CONSTANTS"

static void
set_assignment_type_info(
    C_Compiler* compiler, C_Assignment* assignment, TypeInfo type_info
)
{
    assert(assignment->variable.typing.type < PYTYPE_COUNT);
    if (assignment->variable.typing.type != PYTYPE_UNTYPED &&
        !compare_types(type_info, assignment->variable.typing)) {
        // TODO: some kind of check if this is safe to cast such
        //      ex: if expecting a float, and actually got an int, it's probably safe to
        //      just cast to float

        static const size_t buflen = 1024;
        char expected[buflen];
        char actual[buflen];
        render_type_info_human_readable(assignment->variable.typing, expected, buflen);
        render_type_info_human_readable(type_info, actual, buflen);

        if (assignment->loc) {
            type_errorf(
                compiler->file_index,
                *assignment->loc,
                "trying to assign type `%s` to variable `%s` of type `%s`",
                actual,
                assignment->variable.source_name,
                expected
            );
        }
        else {
            unspecified_errorf(
                compiler->file_index,
                compiler->current_stmt_location,
                "inconsistent typing: expecting `%s` got `%s`",
                expected,
                actual
            );
        }
    }
    else {
        assignment->variable.typing = type_info;
    }
}

static void
prepare_c_assignment_for_rendering(C_Compiler* compiler, C_Assignment* assignment)
{
    if (assignment->variable.final_name) {
        if (!assignment->variable.declared) {
            if (assignment->variable.typing.type == PYTYPE_UNTYPED) UNTYPED_ERROR();
            declare_variable(
                compiler, assignment->variable.typing, assignment->variable.final_name
            );
            assignment->variable.declared = true;
        }
        write_to_section(assignment->section, assignment->variable.final_name);
        write_to_section(assignment->section, " = ");
    }
}

static char*
simple_operand_repr(C_Compiler* compiler, Operand operand)
{
    if (operand.token.type == TOK_IDENTIFIER) {
        // TODO: None should probably be a keyword
        static const SourceString NONE_STR = {.data = "None", .length = 4};
        if (SOURCESTRING_EQ(operand.token.value, NONE_STR)) return "NULL";
        // TODO: should consider a way to avoid this lookup because it happens often
        LexicalScope* scope = scope_stack_peek(&compiler->scope_stack);
        Symbol* sym = symbol_hm_get(&scope->hm, operand.token.value);
        switch (sym->kind) {
            case SYM_SEMI_SCOPED:
                return sym->semi_scoped->current_id.data;
            case SYM_VARIABLE:
                return sym->variable->ns_ident.data;
            case SYM_FUNCTION:
                return sym->func->ns_ident.data;
            case SYM_CLASS:
                return sym->cls->ns_ident.data;
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
                    &compiler->sb, enclosure_assignment->variable.typing.inner->types[0]
                ),
                ");\n",
                NULL
            );
            break;
        case ENCLOSURE_DICT:
            prepare_c_assignment_for_rendering(compiler, enclosure_assignment);
            TypeInfo key_type = enclosure_assignment->variable.typing.inner->types[0];
            write_many_to_section(
                enclosure_assignment->section,
                "DICT_INIT(",
                type_info_to_c_syntax(&compiler->sb, key_type),
                ", ",
                type_info_to_c_syntax(
                    &compiler->sb, enclosure_assignment->variable.typing.inner->types[1]
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
            UNREACHABLE();
    }
}

static void
render_list_literal(
    C_Compiler* compiler, C_Assignment* enclosure_assignment, Operand operand
)
{
    C_Assignment expression_assignment = {.section = enclosure_assignment->section};
    GENERATE_UNIQUE_VAR_NAME(compiler, expression_assignment.variable);

    render_expression_assignment(
        compiler, &expression_assignment, operand.enclosure->expressions[0]
    );

    TypeInfoInner* inner = arena_alloc(compiler->arena, sizeof(TypeInfoInner));
    inner->types = arena_alloc(compiler->arena, sizeof(TypeInfo));
    inner->types[0] = expression_assignment.variable.typing;
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
            enclosure_assignment->variable.final_name,
            ", ",
            type_info_to_c_syntax(&compiler->sb, expression_assignment.variable.typing),
            ", ",
            expression_assignment.variable.final_name,
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
    C_Assignment key_assignment = {.section = enclosure_assignment->section};
    GENERATE_UNIQUE_VAR_NAME(compiler, key_assignment.variable);
    render_expression_assignment(
        compiler, &key_assignment, operand.enclosure->expressions[0]
    );

    C_Assignment val_assignment = {.section = enclosure_assignment->section};
    GENERATE_UNIQUE_VAR_NAME(compiler, val_assignment.variable);
    render_expression_assignment(
        compiler, &val_assignment, operand.enclosure->expressions[1]
    );

    TypeInfoInner* inner = arena_alloc(compiler->arena, sizeof(TypeInfoInner));
    inner->types = arena_alloc(compiler->arena, sizeof(TypeInfo) * 2);
    inner->types[0] = key_assignment.variable.typing;
    inner->types[1] = val_assignment.variable.typing;
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
            enclosure_assignment->variable.final_name,
            ", &",
            key_assignment.variable.final_name,
            ", &",
            val_assignment.variable.final_name,
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
        if (enclosure_assignment->variable.typing.type == PYTYPE_UNTYPED) {
            type_error(
                compiler->file_index,
                compiler->current_stmt_location,
                "empty containers must have their type annotated when initialized"
            );
        }
        render_empty_enclosure(compiler, enclosure_assignment, operand);
        return;
    }

    if (enclosure_assignment->variable.final_name == NULL) {
        // TODO: python allows this but I'm not sure it makes sense for us to allow this
        unspecified_error(
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
            UNREACHABLE();
    }
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

static void
render_callable_args_to_variables(
    C_Compiler* compiler,
    Arguments* args,
    Signature sig,
    char* callable_name,
    C_Assignment* assignments
)
{
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
                sig.params[i].data
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
    C_Assignment assignments[fndef->sig.params_count];
    memset(assignments, 0, sizeof(assignments));
    for (size_t i = 0; i < fndef->sig.params_count; i++) {
        GENERATE_UNIQUE_VAR_NAME(compiler, assignments[i].variable);
        assignments[i].section = assignment->section;
        assignments[i].variable.source_name = fndef->sig.params[i].data;
        assignments[i].variable.typing = fndef->sig.types[i];
    }

    render_callable_args_to_variables(
        compiler, args, fndef->sig, fndef->name.data, assignments
    );

    // write the call statement
    set_assignment_type_info(compiler, assignment, fndef->sig.return_type);
    prepare_c_assignment_for_rendering(compiler, assignment);
    write_to_section(assignment->section, fndef->ns_ident.data);
    write_to_section(assignment->section, "(");
    for (size_t arg_i = 0; arg_i < fndef->sig.params_count; arg_i++) {
        if (arg_i > 0) write_to_section(assignment->section, ", ");
        write_to_section(assignment->section, assignments[arg_i].variable.final_name);
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
    if (!clsdef->fmtstr.data)
        // default fmt string expects all params to be first convereted into cstr
        clsdef->fmtstr = create_default_object_fmt_str(compiler->arena, clsdef);

    C_Assignment as_str_assignments[clsdef->sig.params_count];
    for (size_t i = 0; i < clsdef->sig.params_count; i++) {
        char* member_selector = sb_build_cstr(
            &compiler->sb,
            object_assignment.variable.final_name,
            "->",
            clsdef->sig.params[i].data,
            NULL
        );
        C_Assignment intermediate_member_assignment = {
            .section = object_assignment.section,
            .variable.typing = clsdef->sig.types[i],
            .variable.final_name = member_selector};

        C_Assignment* as_string = as_str_assignments + i;
        GENERATE_UNIQUE_VAR_NAME(compiler, as_string->variable);
        as_string->section = object_assignment.section;
        as_string->variable.typing.type = PYTYPE_STRING;
        convert_assignment_to_string(compiler, intermediate_member_assignment, as_string);
    }

    prepare_c_assignment_for_rendering(compiler, destination);
    write_many_to_section(
        destination->section, "str_fmt(\"", clsdef->fmtstr.data, "\"", NULL
    );
    for (size_t i = 0; i < clsdef->sig.params_count; i++)
        write_many_to_section(
            destination->section,
            ", np_str_to_cstr(",
            as_str_assignments[i].variable.final_name,
            ")",
            NULL
        );
    write_to_section(destination->section, ");\n");
}

static void
convert_object_to_str(
    C_Compiler* compiler, C_Assignment object_assignment, C_Assignment* destination
)
{
    FunctionStatement* user_defined_str =
        object_assignment.variable.typing.cls->object_model_methods[OBJECT_MODEL_STR];
    if (user_defined_str) {
        prepare_c_assignment_for_rendering(compiler, destination);
        write_many_to_section(
            destination->section,
            user_defined_str->ns_ident.data,
            "(",
            object_assignment.variable.final_name,
            ")",
            NULL
        );
    }
    else
        render_default_object_string_represntation(
            compiler,
            object_assignment.variable.typing.cls,
            object_assignment,
            destination
        );
}

static void
convert_assignment_to_string(
    C_Compiler* compiler, C_Assignment assignment_to_convert, C_Assignment* destination
)
{
    if (assignment_to_convert.variable.typing.type == PYTYPE_STRING) return;

    switch (assignment_to_convert.variable.typing.type) {
        case PYTYPE_INT:
            prepare_c_assignment_for_rendering(compiler, destination);
            write_many_to_section(
                destination->section,
                "np_int_to_str(",
                assignment_to_convert.variable.final_name,
                ");\n",
                NULL
            );
            return;
        case PYTYPE_FLOAT:
            prepare_c_assignment_for_rendering(compiler, destination);
            write_many_to_section(
                destination->section,
                "np_float_to_str(",
                assignment_to_convert.variable.final_name,
                ");\n",
                NULL
            );
            return;
        case PYTYPE_BOOL:
            prepare_c_assignment_for_rendering(compiler, destination);
            write_many_to_section(
                destination->section,
                "np_bool_to_str(",
                assignment_to_convert.variable.final_name,
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
                assignment_to_convert.variable.typing, from_type, buflen
            );
            unspecified_errorf(
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
    C_Assignment initial_assignments[args_count];

    if (args->values_count == 0) {
        GENERATE_UNIQUE_VAR_NAME(compiler, initial_assignments[0].variable);
        initial_assignments[0].section = section;
        initial_assignments[0].variable.typing.type = PYTYPE_STRING;
        prepare_c_assignment_for_rendering(compiler, initial_assignments);
        write_to_section(section, "(PYSTRING){.data=\"\", .length=0};\n");
    }
    else {
        for (size_t i = 0; i < args->values_count; i++) {
            GENERATE_UNIQUE_VAR_NAME(compiler, initial_assignments[i].variable);
            initial_assignments[i].section = section;
            initial_assignments[i].variable.typing.type = PYTYPE_UNTYPED;
            render_expression_assignment(
                compiler, initial_assignments + i, args->values[i]
            );
        }
    }

    C_Assignment converted_assignments[args_count];
    for (size_t i = 0; i < args_count; i++) {
        if (initial_assignments[i].variable.typing.type == PYTYPE_STRING)
            converted_assignments[i] = initial_assignments[i];
        else {
            GENERATE_UNIQUE_VAR_NAME(compiler, converted_assignments[i].variable);
            converted_assignments[i].variable.typing.type = PYTYPE_STRING;
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
        write_to_section(section, converted_assignments[i].variable.final_name);
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

    TypeInfo list_content_type = list_assignment->variable.typing.inner->types[0];
    expect_arg_count(compiler, "list.append", args, 1);

    C_Assignment item_assignment = {
        .section = assignment->section,
        .variable.typing = list_content_type,
        .variable.declared = false};
    GENERATE_UNIQUE_VAR_NAME(compiler, item_assignment.variable);
    render_expression_assignment(compiler, &item_assignment, args->values[0]);

    write_many_to_section(
        assignment->section,
        "LIST_APPEND(",
        list_assignment->variable.final_name,
        ", ",
        type_info_to_c_syntax(&compiler->sb, list_content_type),
        ", ",
        item_assignment.variable.final_name,
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
        assignment->section,
        "list_clear(",
        list_assignment->variable.final_name,
        ");\n",
        NULL
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
    set_assignment_type_info(compiler, assignment, list_assignment->variable.typing);

    expect_arg_count(compiler, "list.copy", args, 0);

    prepare_c_assignment_for_rendering(compiler, assignment);
    write_many_to_section(
        assignment->section,
        "list_copy(",
        list_assignment->variable.final_name,
        ");\n",
        NULL
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
    TypeInfo list_content_type = list_assignment->variable.typing.inner->types[0];
    const char* cmp_func = cmp_for_type_info(list_content_type);

    expect_arg_count(compiler, "list.count", args, 1);

    C_Assignment item_assignment = {
        .section = assignment->section, .variable.typing = list_content_type};
    GENERATE_UNIQUE_VAR_NAME(compiler, item_assignment.variable);
    render_expression_assignment(compiler, &item_assignment, args->values[0]);

    TypeInfo return_type = {.type = PYTYPE_INT};
    set_assignment_type_info(compiler, assignment, return_type);
    if (!assignment->variable.declared) {
        declare_variable(compiler, return_type, assignment->variable.final_name);
        assignment->variable.declared = true;
    }

    write_many_to_section(
        assignment->section,
        "LIST_COUNT(",
        list_assignment->variable.final_name,
        ", ",
        type_info_to_c_syntax(&compiler->sb, list_content_type),
        ", ",
        cmp_func,
        ", ",
        item_assignment.variable.final_name,
        ", ",
        assignment->variable.final_name,
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

    C_Assignment other_assignment = {
        .section = assignment->section,
        .variable.typing = list_assignment->variable.typing};
    GENERATE_UNIQUE_VAR_NAME(compiler, other_assignment.variable);
    render_expression_assignment(compiler, &other_assignment, args->values[0]);

    TypeInfo return_type = {.type = PYTYPE_NONE};
    set_assignment_type_info(compiler, assignment, return_type);

    write_many_to_section(
        assignment->section,
        "list_extend(",
        list_assignment->variable.final_name,
        ", ",
        other_assignment.variable.final_name,
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

    TypeInfo list_content_type = list_assignment->variable.typing.inner->types[0];
    const char* cmp = cmp_for_type_info(list_content_type);

    C_Assignment item_assignment = {
        .section = assignment->section, .variable.typing = list_content_type};
    GENERATE_UNIQUE_VAR_NAME(compiler, item_assignment.variable);
    render_expression_assignment(compiler, &item_assignment, args->values[0]);

    TypeInfo return_type = {.type = PYTYPE_INT};
    set_assignment_type_info(compiler, assignment, return_type);

    if (!assignment->variable.final_name) {
        // TODO: maybe trying to call this without assigning it is just an error.
        GENERATE_UNIQUE_VAR_NAME(compiler, assignment->variable);
    }

    if (!assignment->variable.declared) {
        declare_variable(compiler, return_type, assignment->variable.final_name);
        assignment->variable.declared = true;
    }

    write_many_to_section(
        assignment->section,
        "LIST_INDEX(",
        list_assignment->variable.final_name,
        ", ",
        type_info_to_c_syntax(&compiler->sb, list_content_type),
        ", ",
        cmp,
        ", ",
        item_assignment.variable.final_name,
        ", ",
        assignment->variable.final_name,
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

    C_Assignment index_assignment = {
        .section = assignment->section, .variable.typing.type = PYTYPE_INT};
    GENERATE_UNIQUE_VAR_NAME(compiler, index_assignment.variable);
    render_expression_assignment(compiler, &index_assignment, args->values[0]);

    TypeInfo list_content_type = list_assignment->variable.typing.inner->types[0];
    C_Assignment item_assignment = {
        .section = assignment->section, .variable.typing = list_content_type};
    GENERATE_UNIQUE_VAR_NAME(compiler, item_assignment.variable);
    render_expression_assignment(compiler, &item_assignment, args->values[1]);

    TypeInfo return_type = {.type = PYTYPE_NONE};
    set_assignment_type_info(compiler, assignment, return_type);

    write_many_to_section(
        assignment->section,
        "LIST_INSERT(",
        list_assignment->variable.final_name,
        ", ",
        type_info_to_c_syntax(&compiler->sb, list_content_type),
        ", ",
        index_assignment.variable.final_name,
        ", ",
        item_assignment.variable.final_name,
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
    C_Assignment index_assignment = {
        .section = assignment->section, .variable.typing.type = PYTYPE_INT};
    if (args->values_count == 0) {
        index_assignment.variable.final_name = "-1";
    }
    else if (args->values_count == 1) {
        GENERATE_UNIQUE_VAR_NAME(compiler, index_assignment.variable);
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

    TypeInfo list_content_type = list_assignment->variable.typing.inner->types[0];
    set_assignment_type_info(compiler, assignment, list_content_type);
    if (!assignment->variable.final_name) {
        GENERATE_UNIQUE_VAR_NAME(compiler, assignment->variable);
    }
    if (!assignment->variable.declared) {
        declare_variable(compiler, list_content_type, assignment->variable.final_name);
        assignment->variable.declared = true;
    }

    write_many_to_section(
        assignment->section,
        "LIST_POP(",
        list_assignment->variable.final_name,
        ", ",
        type_info_to_c_syntax(&compiler->sb, list_content_type),
        ", ",
        index_assignment.variable.final_name,
        ", ",
        assignment->variable.final_name,
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

    TypeInfo list_content_type = list_assignment->variable.typing.inner->types[0];
    const char* cmp = cmp_for_type_info(list_content_type);

    C_Assignment item_assignment = {
        .section = assignment->section, .variable.typing = list_content_type};
    GENERATE_UNIQUE_VAR_NAME(compiler, item_assignment.variable);
    render_expression_assignment(compiler, &item_assignment, args->values[0]);

    TypeInfo return_type = {.type = PYTYPE_NONE};
    set_assignment_type_info(compiler, assignment, return_type);

    write_many_to_section(
        assignment->section,
        "LIST_REMOVE(",
        list_assignment->variable.final_name,
        ", ",
        type_info_to_c_syntax(&compiler->sb, list_content_type),
        ", ",
        cmp,
        ", ",
        item_assignment.variable.final_name,
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
        assignment->section,
        "list_reverse(",
        list_assignment->variable.final_name,
        ");\n",
        NULL
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
            "ERROR: list.sort expecting either 0 args or boolean keyword arg "
            "`reverse`\n"
        );
        exit(1);
    }

    TypeInfo return_type = {.type = PYTYPE_NONE};
    set_assignment_type_info(compiler, assignment, return_type);

    TypeInfo list_content_type = list_assignment->variable.typing.inner->types[0];
    const char* rev_cmp = sort_cmp_for_type_info(list_content_type, true);
    const char* norm_cmp = sort_cmp_for_type_info(list_content_type, false);

    C_Assignment reversed_assignment = {
        .section = assignment->section, .variable.typing.type = PYTYPE_BOOL};
    if (args->values_count > 0) {
        GENERATE_UNIQUE_VAR_NAME(compiler, reversed_assignment.variable);
        render_expression_assignment(compiler, &reversed_assignment, args->values[0]);
    }
    else {
        reversed_assignment.variable.final_name = "false";
    }

    write_many_to_section(
        assignment->section,
        "LIST_SORT(",
        list_assignment->variable.final_name,
        ", ",
        norm_cmp,
        ", ",
        rev_cmp,
        ", ",
        reversed_assignment.variable.final_name,
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
        assignment->section,
        "dict_clear(",
        dict_assignment->variable.final_name,
        ");\n",
        NULL
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

    TypeInfo return_type = dict_assignment->variable.typing;
    set_assignment_type_info(compiler, assignment, return_type);
    prepare_c_assignment_for_rendering(compiler, assignment);
    write_many_to_section(
        assignment->section,
        "dict_copy(",
        dict_assignment->variable.final_name,
        ");\n",
        NULL
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
        .type = PYTYPE_DICT_ITEMS, .inner = dict_assignment->variable.typing.inner};
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
        dict_assignment->variable.final_name,
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
    return_type.inner->types[0] = dict_assignment->variable.typing.inner->types[0];
    return_type.inner->count = 1;

    set_assignment_type_info(compiler, assignment, return_type);
    prepare_c_assignment_for_rendering(compiler, assignment);
    write_many_to_section(
        assignment->section,
        "dict_iter_keys(",
        dict_assignment->variable.final_name,
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
    TypeInfo return_type = dict_assignment->variable.typing.inner->types[1];
    set_assignment_type_info(compiler, assignment, return_type);

    // TODO: implement default value
    expect_arg_count(compiler, "dict.pop", args, 1);

    C_Assignment key_assignment = {
        .section = assignment->section,
        .variable.typing = dict_assignment->variable.typing.inner->types[0]};
    GENERATE_UNIQUE_VAR_NAME(compiler, key_assignment.variable);
    render_expression_assignment(compiler, &key_assignment, args->values[0]);

    prepare_c_assignment_for_rendering(compiler, assignment);
    write_many_to_section(
        assignment->section,
        "*(",
        type_info_to_c_syntax(&compiler->sb, return_type),
        "*)dict_pop_val(",
        dict_assignment->variable.final_name,
        ", &",
        key_assignment.variable.final_name,
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

    C_Assignment other_dict_assignment = {
        .section = assignment->section,
        .variable.typing = dict_assignment->variable.typing};
    GENERATE_UNIQUE_VAR_NAME(compiler, other_dict_assignment.variable);
    render_expression_assignment(compiler, &other_dict_assignment, args->values[0]);

    write_many_to_section(
        assignment->section,
        "dict_update(",
        dict_assignment->variable.final_name,
        ", ",
        other_dict_assignment.variable.final_name,
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
    return_type.inner->types[0] = dict_assignment->variable.typing.inner->types[1];
    return_type.inner->count = 1;

    set_assignment_type_info(compiler, assignment, return_type);
    prepare_c_assignment_for_rendering(compiler, assignment);
    write_many_to_section(
        assignment->section,
        "dict_iter_vals(",
        dict_assignment->variable.final_name,
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
            assignment->section, fndef->ns_ident.data, "(", operand_reprs[0], ");\n", NULL
        );
    }
    else if (is_rop) {
        write_many_to_section(
            assignment->section,
            fndef->ns_ident.data,
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
            fndef->ns_ident.data,
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
            if (assignment->variable.typing.type == PYTYPE_FLOAT) {
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
                (assignment->variable.typing.type == PYTYPE_INT) ? "(PYINT)" : "",
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
                if (!assignment->variable.declared) {
                    declare_variable(
                        compiler,
                        types[0].inner->types[0],
                        assignment->variable.final_name
                    );
                    assignment->variable.declared = true;
                }
                write_many_to_section(
                    assignment->section,
                    "LIST_GET_ITEM(",
                    operand_reprs[0],
                    ", ",
                    dest_type_c_syntax,
                    ", ",
                    operand_reprs[1],
                    ", ",
                    assignment->variable.final_name,
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
            UNREACHABLE();
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
    C_Assignment intermediate_assignments[sig.params_count];
    memset(intermediate_assignments, 0, sizeof(intermediate_assignments));
    for (size_t i = 0; i < sig.params_count; i++) {
        GENERATE_UNIQUE_VAR_NAME(compiler, intermediate_assignments[i].variable);
        intermediate_assignments[i].section = assignment->section;
        intermediate_assignments[i].variable.typing = fndef->sig.types[i];
        intermediate_assignments[i].variable.source_name = fndef->sig.params[i].data;
    }

    render_callable_args_to_variables(
        compiler, args, sig, fndef->name.data, intermediate_assignments
    );

    // write the call statement
    set_assignment_type_info(compiler, assignment, fndef->sig.return_type);
    prepare_c_assignment_for_rendering(compiler, assignment);
    write_many_to_section(
        assignment->section, fndef->ns_ident.data, "(", self_identifier, NULL
    );
    for (size_t arg_i = 0; arg_i < sig.params_count; arg_i++) {
        write_to_section(assignment->section, ", ");
        write_to_section(
            assignment->section, intermediate_assignments[arg_i].variable.final_name
        );
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
        assignment->section, "np_alloc(sizeof(", clsdef->ns_ident.data, "));\n", NULL
    );

    FunctionStatement* init = clsdef->object_model_methods[OBJECT_MODEL_INIT];
    if (init) {
        for (size_t i = 0; i < clsdef->sig.defaults_count; i++) {
            SourceString varname = sb_build(
                &compiler->sb,
                assignment->variable.final_name,
                "->",
                clsdef->sig.params[clsdef->sig.params_count - 1 - i].data,
                NULL
            );
            C_Assignment default_assignment = {
                .section = assignment->section,
                .variable.typing = clsdef->sig.types[clsdef->sig.params_count - 1 - i],
                .variable.final_name = varname.data,
                .variable.declared = true};
            render_expression_assignment(
                compiler,
                &default_assignment,
                clsdef->sig.defaults[clsdef->sig.defaults_count - 1 - i]
            );
        }
        C_Assignment init_assignment = {
            .section = assignment->section, .variable.typing.type = PYTYPE_NONE};
        render_object_method_call(
            compiler, &init_assignment, assignment->variable.final_name, init, args
        );
        return;
    }

    // var->param1
    // var->param2
    // ...
    C_Assignment intermediate_assignments[clsdef->sig.params_count];
    memset(intermediate_assignments, 0, sizeof(intermediate_assignments));
    for (size_t i = 0; i < clsdef->sig.params_count; i++) {
        char* dst = sb_build_cstr(
            &compiler->sb,
            assignment->variable.final_name,
            "->",
            clsdef->sig.params[i].data,
            NULL
        );
        intermediate_assignments[i].section = assignment->section;
        intermediate_assignments[i].variable.final_name = dst;
        intermediate_assignments[i].variable.source_name = clsdef->sig.params[i].data;
        intermediate_assignments[i].variable.typing = clsdef->sig.types[i];
        intermediate_assignments[i].variable.declared = true;
    }

    // var->param1 = expr1;
    // var->param2 = expr2;
    // ...
    render_callable_args_to_variables(
        compiler, args, clsdef->sig, clsdef->name.data, intermediate_assignments
    );
}

static void
render_call_operation(
    C_Compiler* compiler, C_Assignment* assignment, Expression* expr, Operation operation
)
{
    SourceString fn_identifier = expr->operands[operation.left].token.value;
    Arguments* args = expr->operands[operation.right].args;
    Symbol* sym = symbol_hm_get(&compiler->top_level_scope->hm, fn_identifier);
    if (!sym) {
        render_builtin(compiler, assignment, fn_identifier.data, args);
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
                fn_identifier.data
            );
    }
}

static TypeInfo
get_class_member_type_info(
    C_Compiler* compiler, ClassStatement* clsdef, SourceString member_name
)
{
    for (size_t i = 0; i < clsdef->sig.params_count; i++) {
        if (SOURCESTRING_EQ(member_name, clsdef->sig.params[i]))
            return clsdef->sig.types[i];
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

static void
render_getattr_operation(
    C_Compiler* compiler,
    C_Assignment* assignment,
    char* left_repr,
    TypeInfo left_type,
    SourceString attr
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
    write_many_to_section(assignment->section, left_repr, "->", attr.data, ";\n", NULL);
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
        char variable_memory[2][GENERATED_VARIABLE_CAPACITY] = {0};

        for (size_t lr = (is_unary) ? 1 : 0; lr < 2; lr++) {
            if (operands[lr].kind == OPERAND_TOKEN) {
                operand_reprs[lr] = simple_operand_repr(compiler, operands[lr]);
                operand_types[lr] = resolve_operand_type(&compiler->tc, operands[lr]);
            }
            else {
                WRITE_UNIQUE_VAR_NAME(compiler, variable_memory[lr]);
                C_Assignment operand_assignment = {
                    .section = assignment->section,
                    .variable.final_name = variable_memory[lr],
                    .variable.declared = false};
                render_operand(compiler, &operand_assignment, operands[lr]);
                operand_reprs[lr] = variable_memory[lr];
                operand_types[lr] = operand_assignment.variable.typing;
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
    C_Assignment* assignments;
    C_Assignment** operand_index_to_previous_assignment;
} RenderedOperationMemory;

#define INIT_RENDERED_OPERATION_MEMORY(compiler, struct_varname, expression_ptr)         \
    C_Assignment assignments_memory[expr->operations_count - 1];                         \
    C_Assignment* lookup_memory[expr->operands_count];                                   \
    memset(assignments_memory, 0, sizeof(C_Assignment) * (expr->operations_count - 1));  \
    memset(lookup_memory, 0, sizeof(C_Assignment*) * (expr->operands_count));            \
    for (size_t i = 0; i < expr->operations_count - 1; i++)                              \
        GENERATE_UNIQUE_VAR_NAME(compiler, assignments_memory[i].variable);              \
    struct_varname.assignments = assignments_memory;                                     \
    struct_varname.operand_index_to_previous_assignment = lookup_memory

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
    INIT_RENDERED_OPERATION_MEMORY(compiler, operation_renders, expr);

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
                GENERATE_UNIQUE_VAR_NAME(compiler, left_assignment.variable);
                left_assignment.section = assignment->section;
                render_operand(compiler, &left_assignment, operand);
            }
            if (left_assignment.variable.typing.type == PYTYPE_LIST) {
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
                    expr->operands[operation.right].token.value.data,
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
            else if (left_assignment.variable.typing.type == PYTYPE_DICT) {
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
                    expr->operands[operation.right].token.value.data,
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
            else if (left_assignment.variable.typing.type == PYTYPE_OBJECT) {
                render_getattr_operation(
                    compiler,
                    this_assignment,
                    left_assignment.variable.final_name,
                    left_assignment.variable.typing,
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
        char variable_memory[2][GENERATED_VARIABLE_CAPACITY] = {0};

        bool is_unary =
            (operation.op_type == OPERATOR_LOGICAL_NOT ||
             operation.op_type == OPERATOR_NEGATIVE ||
             operation.op_type == OPERATOR_BITWISE_NOT);

        for (size_t lr = (is_unary) ? 1 : 0; lr < 2; lr++) {
            C_Assignment* previously_rendered =
                operation_renders
                    .operand_index_to_previous_assignment[operand_indices[lr]];
            if (previously_rendered) {
                operand_types[lr] = previously_rendered->variable.typing;
                operand_reprs[lr] = previously_rendered->variable.final_name;
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
                    .variable.final_name = variable_memory[lr],
                    .variable.declared = false};
                render_operand(compiler, &operand_assignment, operand);
                operand_reprs[lr] = variable_memory[lr];
                operand_types[lr] = operand_assignment.variable.typing;
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
    CompilerSection* current_declarations_section =
        compiler->current_variable_declaration_section;
    compiler->current_variable_declaration_section = &compiler->function_definitions;

    CompilerSection* sections[2] = {
        &compiler->function_declarations, &compiler->function_definitions};

    for (size_t i = 0; i < 2; i++) {
        TypeInfo type_info = func->sig.return_type;

        if (type_info.type == PYTYPE_NONE)
            write_to_section(sections[i], "void ");
        else
            write_type_info_to_section(sections[i], &compiler->sb, type_info);

        write_to_section(sections[i], func->ns_ident.data);
        write_to_section(sections[i], "(");
        for (size_t j = 0; j < func->sig.params_count; j++) {
            if (j > 0) write_to_section(sections[i], ", ");
            write_type_info_to_section(sections[i], &compiler->sb, func->sig.types[j]);
            write_to_section(sections[i], func->sig.params[j].data);
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

    compiler->current_variable_declaration_section = current_declarations_section;
}

static void
compile_class(C_Compiler* compiler, ClassStatement* cls)
{
    CompilerSection* current_declarations_section =
        compiler->current_variable_declaration_section;
    compiler->current_variable_declaration_section = &compiler->function_definitions;

    if (cls->sig.params_count == 0) {
        unspecified_error(
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
        write_to_section(&compiler->struct_declarations, cls->sig.params[i].data);
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
                unspecified_error(
                    compiler->file_index,
                    stmt->loc,
                    "only function definitions and annotations are currently "
                    "implemented "
                    "within the body of a class"
                );
        }
    }
    write_many_to_section(
        &compiler->struct_declarations, "} ", cls->ns_ident.data, ";\n", NULL
    );

    compiler->current_variable_declaration_section = current_declarations_section;
}

static void
declare_variable(C_Compiler* compiler, TypeInfo type, char* identifier)
{
    write_many_to_section(
        compiler->current_variable_declaration_section,
        type_info_to_c_syntax(&compiler->sb, type),
        " ",
        identifier,
        ";\n",
        NULL
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
        list_assignment.variable.final_name,
        ", ",
        type_info_to_c_syntax(
            &compiler->sb, list_assignment.variable.typing.inner->types[0]
        ),
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
        dict_assignment.variable.final_name,
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
render_object_op_assignment(
    C_Compiler* compiler,
    C_Assignment obj_assignment,
    C_Assignment other_assignment,
    Operator op_assignment_type
)
{
    ClassStatement* clsdef = obj_assignment.variable.typing.cls;
    ObjectModel om = op_assignment_to_object_model(op_assignment_type);
    FunctionStatement* fndef = clsdef->object_model_methods[om];

    if (!fndef) {
        static const size_t buflen = 1024;
        char object_type_hr[buflen];
        render_type_info_human_readable(
            obj_assignment.variable.typing, object_type_hr, buflen
        );
        type_errorf(
            compiler->file_index,
            compiler->current_stmt_location,
            "type `%s` does not support `%s` operation",
            object_type_hr,
            op_to_cstr(op_assignment_type)
        );
    }
    if (!compare_types(other_assignment.variable.typing, fndef->sig.types[1])) {
        static const size_t buflen = 1024;
        char object_type_hr[buflen];
        render_type_info_human_readable(
            obj_assignment.variable.typing, object_type_hr, buflen
        );
        char rvalue_type_hr[buflen];
        render_type_info_human_readable(
            other_assignment.variable.typing, rvalue_type_hr, buflen
        );
        type_errorf(
            compiler->file_index,
            compiler->current_stmt_location,
            "type `%s` does not support `%s` operation with `%s` rtype",
            object_type_hr,
            op_to_cstr(op_assignment_type),
            rvalue_type_hr
        );
    }

    prepare_c_assignment_for_rendering(compiler, &obj_assignment);
    write_many_to_section(
        obj_assignment.section,
        fndef->ns_ident.data,
        "(",
        obj_assignment.variable.final_name,
        ", ",
        other_assignment.variable.final_name,
        ");\n",
        NULL
    );
}

static void
compile_complex_assignment(
    C_Compiler* compiler, CompilerSection* section, Statement* stmt
)
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
    C_Assignment container_assignment = {.section = section};
    GENERATE_UNIQUE_VAR_NAME(compiler, container_assignment.variable);
    render_expression_assignment(compiler, &container_assignment, &container_expr);

    if (last_op.op_type == OPERATOR_GET_ITEM) {
        // set key/val expected types
        TypeInfo key_type_info;
        TypeInfo val_type_info;
        switch (container_assignment.variable.typing.type) {
            case PYTYPE_LIST: {
                key_type_info.type = PYTYPE_INT;
                val_type_info = container_assignment.variable.typing.inner->types[0];
                break;
            }
            case PYTYPE_DICT: {
                key_type_info = container_assignment.variable.typing.inner->types[0];
                val_type_info = container_assignment.variable.typing.inner->types[1];
                break;
            }
            default:
                UNIMPLEMENTED("setitem not implemented for this data type");
        }

        // render key to a variable
        C_Assignment key_assignment = {
            .section = container_assignment.section, .variable.typing = key_type_info};
        GENERATE_UNIQUE_VAR_NAME(compiler, key_assignment.variable);
        render_operand(compiler, &key_assignment, last_operand);

        C_Assignment val_assignment = {
            .section = container_assignment.section, .variable.typing = val_type_info};
        GENERATE_UNIQUE_VAR_NAME(compiler, val_assignment.variable);

        // render value to a variable
        if (stmt->assignment->op_type == OPERATOR_ASSIGNMENT) {
            render_expression_assignment(
                compiler, &val_assignment, stmt->assignment->value
            );
        }
        else {
            // getitem
            C_Assignment current_val_assignment = {
                .section = container_assignment.section,
                .variable.typing = val_type_info};
            GENERATE_UNIQUE_VAR_NAME(compiler, current_val_assignment.variable);

            char* current_reprs[2] = {
                container_assignment.variable.final_name,
                key_assignment.variable.final_name};
            TypeInfo current_types[2] = {
                container_assignment.variable.typing, key_assignment.variable.typing};

            render_operation(
                compiler,
                &current_val_assignment,
                OPERATOR_GET_ITEM,
                current_reprs,
                current_types
            );

            // right side expression
            C_Assignment other_val_assignment = {.section = container_assignment.section};
            GENERATE_UNIQUE_VAR_NAME(compiler, other_val_assignment.variable);
            render_expression_assignment(
                compiler, &other_val_assignment, stmt->assignment->value
            );

            // render __iadd__, __isub__... object model method
            if (current_val_assignment.variable.typing.type == PYTYPE_OBJECT) {
                current_val_assignment.variable.declared = true;
                render_object_op_assignment(
                    compiler,
                    current_val_assignment,
                    other_val_assignment,
                    stmt->assignment->op_type
                );
                return;
            }

            // combine current value with right side expression
            Operator op_type = OP_ASSIGNMENT_TO_OP_TABLE[stmt->assignment->op_type];
            char* new_reprs[2] = {
                current_val_assignment.variable.final_name,
                other_val_assignment.variable.final_name};
            TypeInfo new_types[2] = {
                current_val_assignment.variable.typing,
                other_val_assignment.variable.typing};
            render_operation(compiler, &val_assignment, op_type, new_reprs, new_types);
        }
        switch (container_assignment.variable.typing.type) {
            case PYTYPE_LIST: {
                render_list_set_item(
                    compiler,
                    container_assignment,
                    key_assignment.variable.final_name,
                    val_assignment.variable.final_name
                );
                return;
            }
            case PYTYPE_DICT: {
                render_dict_set_item(
                    container_assignment,
                    key_assignment.variable.final_name,
                    val_assignment.variable.final_name
                );
                return;
            }
            default:
                UNIMPLEMENTED("setitem not implemented for this data type");
        }
    }
    else if (last_op.op_type == OPERATOR_GET_ATTR) {
        if (container_assignment.variable.typing.type != PYTYPE_OBJECT) {
            UNIMPLEMENTED("setattr for this type not implemented");
        }
        ClassStatement* clsdef = container_assignment.variable.typing.cls;
        TypeInfo member_type =
            get_class_member_type_info(compiler, clsdef, last_operand.token.value);
        SourceString varname = sb_build(
            &compiler->sb,
            container_assignment.variable.final_name,
            "->",
            last_operand.token.value.data,
            NULL
        );
        C_Assignment assignment = {
            .section = container_assignment.section,
            .variable.typing = member_type,
            .variable.final_name = varname.data,
            .variable.declared = true};

        if (stmt->assignment->op_type == OPERATOR_ASSIGNMENT) {
            // regular assignment
            render_expression_assignment(compiler, &assignment, stmt->assignment->value);
        }
        else {
            // op assignment
            Operator op_type = OP_ASSIGNMENT_TO_OP_TABLE[stmt->assignment->op_type];

            // render right hand side to variable
            C_Assignment other_assignment = {.section = container_assignment.section};
            GENERATE_UNIQUE_VAR_NAME(compiler, other_assignment.variable);
            render_expression_assignment(
                compiler, &other_assignment, stmt->assignment->value
            );

            // render op assignment object model method (__iadd__, __isub__, ...)
            if (member_type.type == PYTYPE_OBJECT) {
                render_object_op_assignment(
                    compiler, assignment, other_assignment, stmt->assignment->op_type
                );
            }
            // set member to current_value (op_type) new value
            else {
                // render getattr
                C_Assignment current_val_assignment = {
                    .section = container_assignment.section,
                    .variable.typing = member_type,
                };
                GENERATE_UNIQUE_VAR_NAME(compiler, current_val_assignment.variable);
                render_getattr_operation(
                    compiler,
                    &current_val_assignment,
                    container_assignment.variable.final_name,
                    container_assignment.variable.typing,
                    last_operand.token.value
                );

                // render operation to combine current value with new expression
                char* reprs[2] = {
                    current_val_assignment.variable.final_name,
                    other_assignment.variable.final_name};
                TypeInfo types[2] = {
                    current_val_assignment.variable.typing,
                    other_assignment.variable.typing};
                render_operation(compiler, &assignment, op_type, reprs, types);
            }
        }
        return;
    }
    else
        UNIMPLEMENTED("complex assignment compilation unimplemented for op type");
}

static void
compile_simple_assignment(C_Compiler* compiler, CompilerSection* section, Statement* stmt)
{
    SourceString identifier = stmt->assignment->storage->operands[0].token.value;
    Symbol* sym =
        symbol_hm_get(&scope_stack_peek(&compiler->scope_stack)->hm, identifier);

    TypeInfo* symtype;
    C_Assignment assignment = {.section = section, .loc = &stmt->loc};

    if (sym->kind == SYM_VARIABLE) {
        symtype = &sym->variable->type;
        assignment.variable.final_name = sym->variable->ns_ident.data;
        assignment.variable.source_name = sym->variable->identifier.data;
        assignment.variable.declared = sym->variable->declared;
        assignment.variable.typing = *symtype;
    }
    else if (sym->kind == SYM_SEMI_SCOPED) {
        symtype = &sym->semi_scoped->type;
        assignment.variable.final_name = sym->semi_scoped->current_id.data;
        assignment.variable.source_name = sym->semi_scoped->identifier.data;
        assignment.variable.declared = true;
        assignment.variable.typing = *symtype;
    }
    else
        UNREACHABLE();

    render_expression_assignment(compiler, &assignment, stmt->assignment->value);

    if (symtype->type == PYTYPE_UNTYPED) *symtype = assignment.variable.typing;
    if (sym->kind == SYM_VARIABLE) sym->variable->declared = true;
}

static void
compile_simple_op_assignment(
    C_Compiler* compiler, CompilerSection* section, Statement* stmt
)
{
    SourceString identifier = stmt->assignment->storage->operands[0].token.value;
    Symbol* sym =
        symbol_hm_get(&scope_stack_peek(&compiler->scope_stack)->hm, identifier);
    Operator op_type = OP_ASSIGNMENT_TO_OP_TABLE[stmt->assignment->op_type];

    TypeInfo* symtype;
    C_Assignment assignment = {.section = section, .loc = &stmt->loc};

    if (sym->kind == SYM_VARIABLE) {
        symtype = &sym->variable->type;
        assignment.variable.final_name = sym->variable->ns_ident.data;
        assignment.variable.source_name = sym->variable->identifier.data;
        assignment.variable.typing = *symtype;
        assignment.variable.declared = true;
    }
    else {
        symtype = &sym->semi_scoped->type;
        assignment.variable.final_name = sym->semi_scoped->current_id.data;
        assignment.variable.source_name = sym->semi_scoped->identifier.data;
        assignment.variable.typing = *symtype;
        assignment.variable.declared = true;
    }

    C_Assignment other_assignment = {.section = section};
    GENERATE_UNIQUE_VAR_NAME(compiler, other_assignment.variable);

    render_expression_assignment(compiler, &other_assignment, stmt->assignment->value);

    if (symtype->type == PYTYPE_OBJECT) {
        render_object_op_assignment(
            compiler, assignment, other_assignment, stmt->assignment->op_type
        );
    }
    else {
        char* operand_reprs[2] = {
            assignment.variable.final_name, other_assignment.variable.final_name};
        TypeInfo operand_types[2] = {
            assignment.variable.typing, other_assignment.variable.typing};
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
    LexicalScope* scope = scope_stack_peek(&compiler->scope_stack);
    Symbol* sym = symbol_hm_get(&scope->hm, stmt->annotation->identifier);

    if (sym->kind != SYM_VARIABLE)
        syntax_error(compiler->file_index, stmt->loc, 0, "unexpected annotation");
    if (!sym->variable->declared) {
        declare_variable(compiler, stmt->annotation->type, sym->variable->ns_ident.data);
        sym->variable->declared = true;
    }

    if (stmt->annotation->initial) {
        C_Assignment assignment = {
            .loc = &stmt->loc,
            .section = section,
            .variable.typing = stmt->annotation->type,
            .variable.source_name = sym->variable->identifier.data,
            .variable.final_name = sym->variable->ns_ident.data,
            .variable.declared = true};
        render_expression_assignment(compiler, &assignment, stmt->annotation->initial);
    }
}

static void
compile_return_statement(
    C_Compiler* compiler, CompilerSection* section, ReturnStatement* ret
)
{
    LexicalScope* scope = scope_stack_peek(&compiler->scope_stack);

    C_Assignment assignment = {
        .section = section, .variable.typing = scope->func->sig.return_type};
    GENERATE_UNIQUE_VAR_NAME(compiler, assignment.variable);
    render_expression_assignment(compiler, &assignment, ret->value);

    if (assignment.variable.typing.type == PYTYPE_NONE) {
        write_to_section(section, "return;\n");
    }
    else {
        write_to_section(section, "return ");
        write_to_section(section, assignment.variable.final_name);
        write_to_section(section, ";\n");
    }
}

static char*
init_semi_scoped_variable(
    C_Compiler* compiler, SourceString identifier, TypeInfo type_info
)
{
    LexicalScope* scope = scope_stack_peek(&compiler->scope_stack);
    Symbol* sym = symbol_hm_get(&scope->hm, identifier);
    if (sym->kind == SYM_VARIABLE) return sym->variable->ns_ident.data;
    if (!sym->semi_scoped->current_id.data) {
        sym->semi_scoped->current_id.data =
            arena_alloc(compiler->arena, GENERATED_VARIABLE_CAPACITY);
    }
    WRITE_UNIQUE_VAR_NAME(compiler, sym->semi_scoped->current_id.data);
    sym->semi_scoped->current_id.length = strlen(sym->semi_scoped->current_id.data);
    sym->semi_scoped->type = type_info;
    sym->semi_scoped->directly_in_scope = true;
    declare_variable(compiler, type_info, sym->semi_scoped->current_id.data);
    return sym->semi_scoped->current_id.data;
}

static void
release_semi_scoped_variable(C_Compiler* compiler, SourceString identifier)
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
    SourceString actual_ident = for_loop->it->identifiers[0].name;
    char* it_ident = init_semi_scoped_variable(
        compiler, actual_ident, iterable.variable.typing.inner->types[0]
    );

    // render for loop
    char index_variable[GENERATED_VARIABLE_CAPACITY];
    WRITE_UNIQUE_VAR_NAME(compiler, index_variable);
    write_many_to_section(
        section,
        "LIST_FOR_EACH(",
        iterable.variable.final_name,
        ", ",
        type_info_to_c_syntax(&compiler->sb, iterable.variable.typing.inner->types[0]),
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

    TypeInfo it_type = iterable.variable.typing.inner->types[0];
    SourceString actual_ident = for_loop->it->identifiers[0].name;
    char* it_ident = init_semi_scoped_variable(compiler, actual_ident, it_type);

    // render for loop
    char iter_var[GENERATED_VARIABLE_CAPACITY];
    WRITE_UNIQUE_VAR_NAME(compiler, iter_var);
    write_many_to_section(
        section,
        "DICT_ITER_KEYS(",
        iterable.variable.final_name,
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
    char item_struct_variable[GENERATED_VARIABLE_CAPACITY];
    WRITE_UNIQUE_VAR_NAME(compiler, item_struct_variable);
    write_many_to_section(section, "DictItem ", item_struct_variable, ";\n", NULL);

    // parse key value variable names
    SourceString actual_key_var = {0};
    SourceString actual_val_var = {0};
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
        unspecified_error(
            compiler->file_index,
            compiler->current_stmt_location,
            "expected 2 identifier variable for for dict.items"
        );
    }

    // declare key value variables
    TypeInfoInner* iterable_inner = iterable.variable.typing.inner;
    TypeInfoInner* items_inner = iterable_inner->types[0].inner;
    TypeInfo key_type = items_inner->types[0];
    TypeInfo val_type = items_inner->types[1];

    // put it variables in scope
    char* key_variable = init_semi_scoped_variable(compiler, actual_key_var, key_type);
    char* val_variable = init_semi_scoped_variable(compiler, actual_val_var, val_type);

    // declare void* unpacking variable
    char voidptr_variable[GENERATED_VARIABLE_CAPACITY];
    WRITE_UNIQUE_VAR_NAME(compiler, voidptr_variable);
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
        voidptr_variable," = ",iterable.variable.final_name,".next(", iterable.variable.final_name,".iter)", ",\n",
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
    TypeInfoInner* iterable_inner = iterable.variable.typing.inner;

    if (iterable_inner->count == 1 &&
        iterable_inner->types[0].type == PYTYPE_DICT_ITEMS) {
        render_dict_items_iterator_for_loop_head(compiler, section, for_loop, iterable);
        return;
    }

    if (for_loop->it->identifiers_count > 1 || for_loop->it->identifiers[0].kind != IT_ID)
        UNIMPLEMENTED("for loops with multiple identifiers not currently implemented");

    TypeInfo it_type = iterable.variable.typing.inner->types[0];
    SourceString actual_ident = for_loop->it->identifiers[0].name;
    char* it_ident = init_semi_scoped_variable(compiler, actual_ident, it_type);

    char voidptr_variable[GENERATED_VARIABLE_CAPACITY];
    WRITE_UNIQUE_VAR_NAME(compiler, voidptr_variable);
    write_many_to_section(section, "void* ", voidptr_variable, " = NULL;\n", NULL);

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
        voidptr_variable," = ",iterable.variable.final_name,".next(", iterable.variable.final_name,".iter)", ",\n",
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
    temporary_section(section)
    {
        C_Assignment iterable = {.section = section};
        GENERATE_UNIQUE_VAR_NAME(compiler, iterable.variable);
        render_expression_assignment(compiler, &iterable, for_loop->iterable);

        if (iterable.variable.typing.type == PYTYPE_LIST)
            render_list_for_each_head(compiler, section, for_loop, iterable);
        else if (iterable.variable.typing.type == PYTYPE_DICT)
            render_dict_for_each_head(compiler, section, for_loop, iterable);
        else if (iterable.variable.typing.type == PYTYPE_ITER)
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
        for (size_t i = 0; i < for_loop->body.stmts_count; i++)
            compile_statement(compiler, section, for_loop->body.stmts[i]);
        write_to_section(section, "}\n");
    }
}

static char*
assignment_to_truthy(StringBuilder* sb, C_Assignment assignment)
{
    switch (assignment.variable.typing.type) {
        case PYTYPE_INT:
            return assignment.variable.final_name;
        case PYTYPE_FLOAT:
            return assignment.variable.final_name;
        case PYTYPE_BOOL:
            return assignment.variable.final_name;
        case PYTYPE_STRING:
            return sb_build_cstr(sb, assignment.variable.final_name, ".length", NULL);
        case PYTYPE_LIST:
            return sb_build_cstr(sb, assignment.variable.final_name, "->count", NULL);
        case PYTYPE_DICT:
            return sb_build_cstr(sb, assignment.variable.final_name, "->count", NULL);
        case PYTYPE_NONE:
            return "0";
        case PYTYPE_ITER:
            UNIMPLEMENTED("truthy conversion unimplemented for PYTYPE_ITER");
        case PYTYPE_OBJECT:
            UNIMPLEMENTED("truthy conversion unimplemented for PYTYPE_OBJECT");
        case PYTYPE_SLICE:
            UNREACHABLE();
        case PYTYPE_DICT_ITEMS:
            UNREACHABLE();
        case PYTYPE_UNTYPED:
            UNREACHABLE();
        case PYTYPE_TUPLE:
            UNREACHABLE();
        case PYTYPE_COUNT:
            UNREACHABLE();
    }
    UNREACHABLE();
}

static void
compile_if(
    C_Compiler* compiler, CompilerSection* section, ConditionalStatement* conditional
)
{
    temporary_section(section)
    {
        GENERATE_UNIQUE_GOTO_IDENTIFIER(compiler, goto_ident);
        bool req_goto =
            (conditional->else_body.stmts_count > 0 || conditional->elifs_count > 0);

        // if
        C_Assignment condition_assignment = {.section = section};
        GENERATE_UNIQUE_VAR_NAME(compiler, condition_assignment.variable);
        render_expression_assignment(
            compiler, &condition_assignment, conditional->condition
        );
        write_many_to_section(
            section,
            "if (",
            assignment_to_truthy(&compiler->sb, condition_assignment),
            ") {\n",
            NULL
        );
        for (size_t i = 0; i < conditional->body.stmts_count; i++)
            compile_statement(compiler, section, conditional->body.stmts[i]);
        if (req_goto) write_many_to_section(section, "goto ", goto_ident, ";\n", NULL);
        write_to_section(section, "}\n");

        // elifs
        for (size_t i = 0; i < conditional->elifs_count; i++) {
            C_Assignment elif_condition_assignment = {.section = section};
            GENERATE_UNIQUE_VAR_NAME(compiler, elif_condition_assignment.variable);
            render_expression_assignment(
                compiler, &elif_condition_assignment, conditional->elifs[i].condition
            );
            write_many_to_section(
                section,
                "if (",
                assignment_to_truthy(&compiler->sb, elif_condition_assignment),
                ") {\n",
                NULL
            );
            for (size_t stmt_i = 0; stmt_i < conditional->elifs[i].body.stmts_count;
                 stmt_i++)
                compile_statement(
                    compiler, section, conditional->elifs[i].body.stmts[stmt_i]
                );
            write_many_to_section(section, "goto ", goto_ident, ";\n", NULL);
            write_to_section(section, "}\n");
        }

        // else
        if (conditional->else_body.stmts_count > 0) {
            for (size_t i = 0; i < conditional->else_body.stmts_count; i++)
                compile_statement(compiler, section, conditional->else_body.stmts[i]);
        }
        if (req_goto) write_many_to_section(section, goto_ident, ":\n", NULL);
    }
}

static void
compile_while(C_Compiler* compiler, CompilerSection* section, WhileStatement* while_stmt)
{
    temporary_section(section)
    {
        write_to_section(section, "for(;;) {\n");

        C_Assignment condition_assignment = {.section = section};
        GENERATE_UNIQUE_VAR_NAME(compiler, condition_assignment.variable);
        render_expression_assignment(
            compiler, &condition_assignment, while_stmt->condition
        );
        write_many_to_section(
            section,
            "if (!",
            assignment_to_truthy(&compiler->sb, condition_assignment),
            ") break;\n",
            NULL
        );
        for (size_t i = 0; i < while_stmt->body.stmts_count; i++)
            compile_statement(compiler, section, while_stmt->body.stmts[i]);

        write_to_section(section, "}\n");
    }
}

static void
compile_statement(C_Compiler* compiler, CompilerSection* section_or_null, Statement* stmt)
{
    compiler->current_stmt_location = stmt->loc;
    CompilerSection* section =
        (section_or_null) ? section_or_null : &compiler->init_module_function;

    switch (stmt->kind) {
        case STMT_FOR_LOOP:
            compile_for_loop(compiler, section, stmt->for_loop);
            break;
        case STMT_IMPORT:
            UNIMPLEMENTED("import compilation is unimplemented");
        case STMT_WHILE:
            compile_while(compiler, section, stmt->while_loop);
            break;
        case STMT_IF:
            compile_if(compiler, section, stmt->conditional);
            break;
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
        case STMT_ASSIGNMENT:
            compile_assignment(compiler, section, stmt);
            break;
        case STMT_ANNOTATION:
            compile_annotation(compiler, section, stmt);
            break;
        case STMT_EXPR: {
            C_Assignment assignment = {
                .section = section,
                .variable.final_name = NULL,
                .variable.declared = false};
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
            UNREACHABLE();
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
        SourceString str = compiler->str_hm.elements[i];
        if (i > 0) write_to_section(&compiler->forward, ",\n");
        write_to_section(&compiler->forward, "{.data=\"");
        write_to_section(&compiler->forward, str.data);
        write_to_section(&compiler->forward, "\", .length=");
        char length_as_str[10];
        sprintf(length_as_str, "%zu", str.length);
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
    compiler.current_variable_declaration_section = &compiler.variable_declarations;
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
