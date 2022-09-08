#include "c_compiler.h"

#include <assert.h>
#include <string.h>

#include "hash.h"
#include "lexer_helpers.h"
#include "not_python.h"
#include "type_checker.h"

#define UNREACHABLE(msg) assert(0 && msg);

// TODO: some standard way to implement name mangling will be needed at some point
// TODO: tracking if variables have been initialized before use and warning if not

typedef struct {
    size_t capacity;
    size_t count;
    StringView* elements;
    int* lookup;
} StringHashmap;

static void
str_hm_rehash(StringHashmap* hm)
{
    for (size_t i = 0; i < hm->count; i++) {
        StringView str = hm->elements[i];
        uint64_t hash = hash_bytes(str.data + str.offset, str.length);
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
    hm->elements = realloc(hm->elements, sizeof(StringView) * hm->capacity);
    hm->lookup = realloc(hm->lookup, sizeof(int) * 2 * hm->capacity);
    if (!hm->elements || !hm->lookup) out_of_memory();
    memset(hm->lookup, -1, sizeof(int) * 2 * hm->capacity);
    str_hm_rehash(hm);
}

static size_t
str_hm_put(StringHashmap* hm, StringView element)
{
    if (hm->count == hm->capacity) str_hm_grow(hm);
    uint64_t hash = hash_bytes(element.data + element.offset, element.length);
    size_t probe = hash % (hm->capacity * 2);
    for (;;) {
        int index = hm->lookup[probe];
        if (index == -1) {
            hm->lookup[probe] = hm->count;
            hm->elements[hm->count] = element;
            return hm->count++;
        }
        StringView existing = hm->elements[index];
        if (str_eq(element, existing)) return (size_t)index;
        if (probe == hm->capacity * 2)
            probe = 0;
        else
            probe++;
    }
}

static int
str_hm_put_cstr(StringHashmap* hm, char* element)
{
    StringView sv = {.data = element, .offset = 0, .length = strlen(element)};
    return str_hm_put(hm, sv);
}

static void
str_hm_free(StringHashmap* hm)
{
    free(hm->elements);
    free(hm->lookup);
}

typedef struct {
    size_t capacity;
    size_t count;
    char** elements;
} IdentifierSet;

static void
id_set_rehash(char** old_buffer, size_t old_count, char** new_buffer, size_t new_count)
{
    // during rehash we know elements are unique already
    for (size_t i = 0; i < old_count; i++) {
        char* id = old_buffer[i];
        if (!id) continue;
        uint64_t hash = hash_bytes(id, strlen(id));
        size_t probe = hash % new_count;
        for (;;) {
            if (!new_buffer[probe]) {
                new_buffer[probe] = id;
                break;
            }
            if (probe == new_count - 1)
                probe = 0;
            else
                probe++;
        }
    }
}

// returns true if it added a new element or false if element already exists
static bool
id_set_add(IdentifierSet* set, char* id)
{
    if (set->count >= set->capacity / 2) {
        size_t old_capacity = set->capacity;
        if (set->capacity == 0)
            set->capacity = 8;
        else
            set->capacity *= 2;
        char** new_buffer = calloc(set->capacity, sizeof(char*));
        id_set_rehash(set->elements, old_capacity, new_buffer, set->capacity);
        set->elements = new_buffer;
    }
    uint64_t hash = hash_bytes(id, strlen(id));
    size_t probe = hash % set->capacity;
    for (;;) {
        char* elem = set->elements[probe];
        if (!elem) {
            set->elements[probe] = id;
            set->count += 1;
            return true;
        }
        else if (strcmp(id, elem) == 0)
            return false;
        if (probe == set->capacity - 1)
            probe = 0;
        else
            probe++;
    }
}

static void
id_set_free(IdentifierSet* set)
{
    free(set->elements);
}

typedef struct {
    size_t capacity;
    size_t remaining;
    char* buffer;
    char* write;
} CompilerSection;

static void
section_grow(CompilerSection* section)
{
    if (section->capacity == 0) {
        section->capacity = 128;
        section->remaining = 128;
    }
    else {
        section->remaining += section->capacity;
        section->capacity *= 2;
    }
    section->buffer = realloc(section->buffer, section->capacity);
    section->write = section->buffer + section->capacity - section->remaining;
    if (!section->buffer) out_of_memory();
    memset(section->write, 0, section->remaining);
}

static void
section_free(CompilerSection* section)
{
    free(section->buffer);
}

static void
write_to_section(CompilerSection* section, char* data)
{
    char c;
    while ((c = *(data++)) != '\0') {
        if (section->remaining <= 1) section_grow(section);
        *(section->write++) = c;
        section->remaining -= 1;
    }
}

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
} C_Compiler;

// TODO: unique vars should probably be namespaced by module in the future

#define UNIQUE_VAR_LENGTH 12

// TODO: I'd prefer a change that allows this macro to accept `;` at the end
#define DEFINE_UNIQUE_VARS(compiler_ptr, count, name)                                    \
    char name[count][UNIQUE_VAR_LENGTH];                                                 \
    for (size_t i = 0; i < count; i++) {                                                 \
        sprintf(name[i], "NP_var%zu", compiler_ptr->unique_vars_counter++);              \
    }

#define DEFINE_UNIQUE_VAR(compiler_ptr, name)                                            \
    char name[UNIQUE_VAR_LENGTH];                                                        \
    sprintf(name, "NP_var%zu", compiler_ptr->unique_vars_counter++)

#define WRITE_UNIQUE_VAR(compiler_ptr, dest)                                             \
    sprintf(dest, "NP_var%zu", compiler_ptr->unique_vars_counter++)

#define DATATYPE_INT "PYINT"
#define DATATYPE_FLOAT "PYFLOAT"
#define DATATYPE_STRING "PYSTRING"

#define STRING_CONSTANTS_TABLE_NAME "NOT_PYTHON_STRING_CONSTANTS"

static void
untyped_error(void)
{
    // TODO: locations of tokens should be available here to give a reasonable error
    // should it occur
    fprintf(stderr, "encountered untyped variable\n");
    exit(1);
}

static bool
is_simple_expression(Expression* expr)
{
    return (expr->operations_count == 0 && expr->operands[0].kind == OPERAND_TOKEN);
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
        case PYTYPE_LIST:
            UNIMPLEMENTED("list to c syntax unimplemented");
            break;
        case PYTYPE_TUPLE:
            UNIMPLEMENTED("tuple to c syntax unimplemented");
            break;
        case PYTYPE_DICT:
            UNIMPLEMENTED("dict to c syntax unimplemented");
            break;
        case PYTYPE_OBJECT:
            UNIMPLEMENTED("object to c syntax unimplemented");
            break;
        default:
            UNREACHABLE("default type info to c syntax");
            break;
    }
}

static void
write_simple_operand_to_section(
    C_Compiler* compiler, CompilerSection* section, Operand operand
)
{
    if (operand.token.type == TOK_IDENTIFIER || operand.token.type == TOK_NUMBER) {
        write_to_section(section, operand.token.value);
    }
    else if (operand.token.type == TOK_STRING) {
        size_t index = str_hm_put_cstr(&compiler->str_hm, operand.token.value);
        char strindex[10];
        sprintf(strindex, "%zu", index);
        write_to_section(section, STRING_CONSTANTS_TABLE_NAME "[");
        write_to_section(section, strindex);
        write_to_section(section, "]");
    }
    else
        UNREACHABLE("unexpected simple operand");
}

static TypeInfo
write_simple_expression_to_section(
    C_Compiler* compiler,
    CompilerSection* section,
    Expression* expr,
    char* destination,
    bool dest_is_declared
)
{
    assert(expr->operations_count == 0);
    Operand operand = expr->operands[0];

    TypeInfo expr_type = resolve_operand_type(&compiler->tc, operand);
    if (destination) {
        if (!dest_is_declared) write_type_info_to_section(section, expr_type);
        write_to_section(section, destination);
        write_to_section(section, " = ");
    }
    write_simple_operand_to_section(compiler, section, operand);
    write_to_section(section, ";\n");

    return expr_type;
}

static void compile_statement(
    C_Compiler* compiler, CompilerSection* section, Statement* stmt
);

static TypeInfo render_expression(
    C_Compiler* compiler,
    CompilerSection* section,
    Expression* expr,
    char* destination,
    bool dest_is_declared
);

// -1 on bad kwd
// TODO: should check if parser even allows this to happen in the future
static int
index_of_kwarg(FunctionStatement* fndef, char* kwd)
{
    for (size_t i = 0; i < fndef->sig.params_count; i++) {
        if (strcmp(kwd, fndef->sig.params[i]) == 0) return i;
    }
    return -1;
}

static void
write_typed_expr_to_variable(
    C_Compiler* compiler,
    CompilerSection* section,
    Expression* expr,
    TypeInfo type,
    char* destination
)
{
    TypeInfo checked_type =
        render_expression(compiler, section, expr, destination, false);
    if (!compare_types(type, checked_type)) {
        // TODO: better error reporting
        fprintf(stderr, "ERROR: inconsistent typing\n");
        exit(1);
    }
}

static void
write_function_call_to_section(
    C_Compiler* compiler,
    CompilerSection* section,
    char* destination,
    bool dest_is_declared,
    FunctionStatement* fndef,
    Arguments* args
)
{
    if (args->values_count > fndef->sig.params_count) {
        // TODO: better error reporting
        fprintf(stderr, "ERROR: too many arguments provided\n");
        exit(1);
    }

    // intermediate variables to store args into
    DEFINE_UNIQUE_VARS(compiler, fndef->sig.params_count, param_vars)

    bool params_fulfilled[fndef->sig.params_count];
    memset(params_fulfilled, true, sizeof(bool) * args->n_positional);
    memset(
        params_fulfilled + args->n_positional,
        false,
        sizeof(bool) * (fndef->sig.params_count - args->n_positional)
    );

    // parse positional args
    for (size_t arg_i = 0; arg_i < args->n_positional; arg_i++) {
        Expression* arg_value = args->values[arg_i];
        TypeInfo arg_type = fndef->sig.types[arg_i];
        write_typed_expr_to_variable(
            compiler, section, arg_value, arg_type, param_vars[arg_i]
        );
    }

    // parse kwargs
    for (size_t i = args->n_positional; i < args->values_count; i++) {
        int kwd_index = index_of_kwarg(fndef, args->kwds[i - args->n_positional]);
        if (kwd_index < 0) {
            // TODO: better error message with location diagnostics
            fprintf(stderr, "ERROR: bad keyword argument\n");
            exit(1);
        }
        params_fulfilled[kwd_index] = true;

        Expression* arg_value = args->values[i];
        TypeInfo arg_type = fndef->sig.types[kwd_index];
        write_typed_expr_to_variable(
            compiler, section, arg_value, arg_type, param_vars[kwd_index]
        );
    }

    // check that all required params are fulfilled
    size_t required_count = fndef->sig.params_count - fndef->sig.defaults_count;
    for (size_t i = 0; i < required_count; i++) {
        if (!params_fulfilled[i]) {
            // TODO: better error reporting
            fprintf(stderr, "ERROR: required param not provided\n");
            exit(1);
        }
    }

    // fill in any unprovided values with their default expressions
    for (size_t i = required_count; i < fndef->sig.params_count; i++) {
        if (params_fulfilled[i])
            continue;
        else {
            Expression* arg_value = fndef->sig.defaults[i - required_count];
            TypeInfo arg_type = fndef->sig.types[i];
            write_typed_expr_to_variable(
                compiler, section, arg_value, arg_type, param_vars[i]
            );
        }
    }

    // write the call statement
    if (destination) {
        if (!dest_is_declared)
            write_type_info_to_section(section, fndef->sig.return_type);
        write_to_section(section, destination);
        write_to_section(section, " = ");
    }
    write_to_section(section, fndef->name);
    write_to_section(section, "(");
    for (size_t arg_i = 0; arg_i < fndef->sig.params_count; arg_i++) {
        if (arg_i > 0) write_to_section(section, ", ");
        write_to_section(section, param_vars[arg_i]);
    }
    write_to_section(section, ");\n");
}

static TypeInfo
render_builtin_print(C_Compiler* compiler, CompilerSection* section, Arguments* args)
{
    if (args->values_count != args->n_positional) {
        // TODO: error message
        fprintf(stderr, "ERROR: print doesn't take keyword arguments\n");
        exit(1);
    }
    DEFINE_UNIQUE_VARS(compiler, args->values_count, string_vars)
    for (size_t i = 0; i < args->values_count; i++) {
        TypeInfo arg_type =
            render_expression(compiler, section, args->values[i], string_vars[i], false);
        if (arg_type.type != PYTYPE_STRING) {
            // TODO: error message
            fprintf(stderr, "ERROR: print only takes strings as arguments\n");
            exit(1);
        }
    }
    char arg_count_as_str[10] = {0};
    sprintf(arg_count_as_str, "%zu", args->values_count);
    write_to_section(section, "builtin_print(");
    write_to_section(section, arg_count_as_str);
    write_to_section(section, ", ");
    for (size_t i = 0; i < args->values_count; i++) {
        if (i > 0) write_to_section(section, ", ");
        write_to_section(section, string_vars[i]);
    }
    write_to_section(section, ");\n");
    return (TypeInfo){.type = PYTYPE_NONE};
}

// TODO: parser will need to enforce that builtins dont get defined by the user
static TypeInfo
render_builtin(
    C_Compiler* compiler,
    CompilerSection* section,
    char* destination,
    bool dest_is_declared,
    char* fn_identifier,
    Arguments* args
)
{
    (void)destination;
    (void)dest_is_declared;
    if (strcmp(fn_identifier, "print") == 0) {
        render_builtin_print(compiler, section, args);
        return (TypeInfo){.type = PYTYPE_NONE};
    }
    else {
        // TODO: better error message
        fprintf(stderr, "ERROR: function undefined (%s)\n", fn_identifier);
        exit(1);
    }
}

/*
 * This function currently creates a new scope and renders each
 * operation of an expression into a variable on a new line and
 * assigns the final value to the destination at the end. I may
 * want to revisit this after the rest of the compiler is implemented.
 */
static TypeInfo
render_expression(
    C_Compiler* compiler,
    CompilerSection* section,
    Expression* expr,
    char* destination,
    bool dest_is_declared
)
{
    if (is_simple_expression(expr))
        return write_simple_expression_to_section(
            compiler, section, expr, destination, dest_is_declared
        );
    // when rendering: (1 + 2 * 3 + 4)
    // first 2 * 3 is rendered
    // next 1 + 2 must know what (2) now refers to the result of (2 * 3)
    // these variables are used to facilitate this kind of logic
    DEFINE_UNIQUE_VARS(compiler, expr->operations_count - 1, intermediate_variables)
    TypeInfo resolved_operation_types[expr->operations_count];
    size_t size_t_ptr_memory[expr->operands_count];
    size_t* operand_to_resolved_operation_index[expr->operands_count];
    memset(resolved_operation_types, 0, sizeof(TypeInfo) * expr->operations_count);
    memset(
        operand_to_resolved_operation_index, 0, sizeof(size_t*) * expr->operands_count
    );

    for (size_t i = 0; i < expr->operations_count; i++) {
        char* this_operation_dest =
            (i == expr->operations_count - 1) ? destination : intermediate_variables[i];
        Operation operation = expr->operations[i];

        if (operation.op_type == OPERATOR_CALL) {
            // TODO: does not work for methods
            char* fn_identifier = expr->operands[operation.left].token.value;
            Arguments* args = expr->operands[operation.right].args;
            Symbol* sym = symbol_hm_get(&compiler->top_level_scope->hm, fn_identifier);
            if (!sym) {
                return render_builtin(
                    compiler,
                    section,
                    this_operation_dest,
                    dest_is_declared,
                    fn_identifier,
                    args
                );
            }
            FunctionStatement* fndef = sym->func;
            resolved_operation_types[i] = fndef->sig.return_type;

            write_function_call_to_section(
                compiler, section, this_operation_dest, dest_is_declared, fndef, args
            );

            size_t_ptr_memory[operation.left] = i;
            size_t_ptr_memory[operation.right] = i;
            operand_to_resolved_operation_index[operation.left] =
                size_t_ptr_memory + operation.left;
            operand_to_resolved_operation_index[operation.right] =
                size_t_ptr_memory + operation.right;
            continue;
        }

        size_t operand_indices[2] = {operation.left, operation.right};
        TypeInfo operand_types[2] = {0};
        // If we encounter an expression operand we will assign its value into
        // an intermediate variable. `as_variable` is to remember the var name.
        char as_variable[2][12] = {0};

        bool is_unary =
            (operation.op_type == OPERATOR_LOGICAL_NOT ||
             operation.op_type == OPERATOR_NEGATIVE ||
             operation.op_type == OPERATOR_BITWISE_NOT);

        for (size_t j = (is_unary) ? 1 : 0; j < 2; j++) {
            size_t* resolved_ref =
                operand_to_resolved_operation_index[operand_indices[j]];
            if (resolved_ref) {
                operand_types[j] = resolved_operation_types[*resolved_ref];
                continue;
            }

            Operand operand = expr->operands[operand_indices[j]];
            switch (operand.kind) {
                case OPERAND_ENCLOSURE_LITERAL:
                    UNIMPLEMENTED("enclosure literal rendering unimplemented");
                    break;
                case OPERAND_COMPREHENSION:
                    UNIMPLEMENTED("comprehension rendering unimplemented");
                    break;
                case OPERAND_SLICE:
                    UNIMPLEMENTED("slice rendering unimplemented");
                    break;
                case OPERAND_EXPRESSION: {
                    WRITE_UNIQUE_VAR(compiler, as_variable[j]);
                    operand_types[j] = render_expression(
                        compiler, section, operand.expr, as_variable[j], false
                    );
                    break;
                }
                case OPERAND_TOKEN:
                    operand_types[j] = resolve_operand_type(&compiler->tc, operand);
                    break;
                default:
                    UNREACHABLE(
                        "default case in expression rendering should not be reached"
                    );
                    break;
            }
        }
        resolved_operation_types[i] =
            resolve_operation_type(operand_types[0], operand_types[1], operation.op_type);
        if (this_operation_dest) {
            // on the final assignment, if the destination is already declared, dont add
            // type
            if (i != expr->operations_count - 1 || !dest_is_declared)
                write_type_info_to_section(section, resolved_operation_types[i]);
            write_to_section(section, this_operation_dest);
            write_to_section(section, " = ");
        }

        if (!is_unary) {
            size_t* left_ref = operand_to_resolved_operation_index[operation.left];
            if (left_ref) {
                write_to_section(section, intermediate_variables[*left_ref]);
                *left_ref = i;
            }
            else {
                if (as_variable[0][0] != '\0') {
                    write_to_section(section, as_variable[0]);
                }
                else {
                    write_simple_operand_to_section(
                        compiler, section, expr->operands[operation.left]
                    );
                }
                size_t* new_ref = size_t_ptr_memory + operation.left;
                *new_ref = i;
                operand_to_resolved_operation_index[operation.left] = new_ref;
            }
        }

        // TODO: this won't work for all operators
        write_to_section(section, (char*)op_to_cstr(operation.op_type));

        size_t* right_ref = operand_to_resolved_operation_index[operation.right];
        if (right_ref) {
            write_to_section(section, intermediate_variables[*right_ref]);
            *right_ref = i;
        }
        else {
            if (as_variable[1][0] != '\0') {
                write_to_section(section, as_variable[1]);
            }
            else {
                write_simple_operand_to_section(
                    compiler, section, expr->operands[operation.right]
                );
            }
            size_t* new_ref = size_t_ptr_memory + operation.right;
            *new_ref = i;
            operand_to_resolved_operation_index[operation.right] = new_ref;
        }

        write_to_section(section, ";\n");
    }

    TypeInfo final_type = resolved_operation_types[expr->operations_count - 1];
    return final_type;
}

static void
compile_function(C_Compiler* compiler, FunctionStatement* func)
{
    CompilerSection* sections[2] = {
        &compiler->function_declarations, &compiler->function_definitions};

    for (size_t i = 0; i < 2; i++) {
        write_type_info_to_section(sections[i], func->sig.return_type);
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
        // TODO: better error reporting or perhaps this should be an error from the parser
        // also worth considering if classes can exist without members as namespaces for
        // functions
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
compile_assignment(
    C_Compiler* compiler, CompilerSection* section, AssignmentStatement* assignment
)
{
    if (assignment->op_type != OPERATOR_ASSIGNMENT)
        UNIMPLEMENTED("assignment op_type not implemented");
    if (!is_simple_expression(assignment->storage))
        UNIMPLEMENTED("assignment to non-variables not yet implemented");

    char* identifier = assignment->storage->operands[0].token.value;
    TypeInfo expr_type =
        render_expression(compiler, section, assignment->value, identifier, true);
    if (section == &compiler->init_module_function)
        declare_global_variable(compiler, expr_type, identifier);
}

static void
compile_annotation(
    C_Compiler* compiler, CompilerSection* section, AnnotationStatement* annotation
)
{
    // TODO: implement default values for class members
    if (scope_stack_peek(&compiler->scope_stack) == compiler->top_level_scope)
        declare_global_variable(compiler, annotation->type, annotation->identifier);

    if (annotation->initial) {
        render_expression(
            compiler, section, annotation->initial, annotation->identifier, true
        );
    }
}

static void
compile_expression(
    C_Compiler* compiler, CompilerSection* section, Expression* expr, char* destination
)
{
    render_expression(compiler, section, expr, destination, false);
}

static void
compile_return_statement(
    C_Compiler* compiler, CompilerSection* section, ReturnStatement* ret
)
{
    DEFINE_UNIQUE_VAR(compiler, return_var);
    compile_expression(compiler, section, ret->value, return_var);
    write_to_section(section, "return ");
    write_to_section(section, return_var);
    write_to_section(section, ";\n");
}

static void
compile_statement(C_Compiler* compiler, CompilerSection* section_or_null, Statement* stmt)
{
    switch (stmt->kind) {
        case STMT_FOR_LOOP:
            UNIMPLEMENTED("for loop compilation is unimplemented");
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
            compile_expression(compiler, section, stmt->expr, NULL);
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
            UNIMPLEMENTED("null statement is unimplemented");
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
        .arena = lexer->arena, .top_level_scope = lexer->top_level, .tc = tc};
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
    section_free(&compiler.forward);
    section_free(&compiler.variable_declarations);
    section_free(&compiler.struct_declarations);
    section_free(&compiler.function_declarations);
    section_free(&compiler.function_definitions);
    section_free(&compiler.init_module_function);
    section_free(&compiler.main_function);
}
