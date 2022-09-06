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
} C_Compiler;

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

typedef struct {
    // a block that renders each expression operation to a temporary variable
    // and assigns the result to a variable that must have been already
    // declared outside the scope
    CompilerSection section;
    // the type info of the fully resolved expression
    TypeInfo final_type;
} ExpressionBlock;

static void
expression_block_free(ExpressionBlock block)
{
    section_free(&block.section);
}

/*
 * This function currently creates a new scope and renders each
 * operation of an expression into a variable on a new line and
 * assigns the final value to the destination at the end. I may
 * want to revisit this after the rest of the compiler is implemented.
 */
static ExpressionBlock
render_expression(C_Compiler* compiler, Expression* expr, char* destination)
{
    ExpressionBlock block = {0};
    write_to_section(&block.section, "{\n");

    size_t intermediate_id_counter = 0;
    char intermediate_variables[expr->operations_count][10];
    for (size_t i = 0; i < expr->operations_count; i++) {
        sprintf(intermediate_variables[i], "var%zu", intermediate_id_counter++);
    }

    TypeInfo resolved_operation_types[expr->operations_count];
    size_t size_t_values[expr->operands_count];  // static size_t memory for the following
                                                 // pointer array
    size_t* operand_to_resolved_operations[expr->operands_count];
    memset(resolved_operation_types, 0, sizeof(TypeInfo) * expr->operations_count);
    memset(operand_to_resolved_operations, 0, sizeof(size_t*) * expr->operands_count);

    for (size_t i = 0; i < expr->operations_count; i++) {
        Operation operation = expr->operations[i];
        size_t operand_indices[2] = {operation.left, operation.right};
        TypeInfo operand_types[2] = {0};
        char as_variable[2][10] = {0};

        bool is_unary =
            (operation.op_type == OPERATOR_LOGICAL_NOT ||
             operation.op_type == OPERATOR_NEGATIVE ||
             operation.op_type == OPERATOR_BITWISE_NOT);

        for (size_t j = (is_unary) ? 1 : 0; j < 2; j++) {
            size_t* resolved_ref = operand_to_resolved_operations[operand_indices[j]];
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
                case OPERAND_ARGUMENTS:
                    UNIMPLEMENTED("arguments rendering unimplemented");
                    break;
                case OPERAND_SLICE:
                    UNIMPLEMENTED("slice rendering unimplemented");
                    break;
                case OPERAND_EXPRESSION: {
                    sprintf(as_variable[j], "var%zu", intermediate_id_counter++);
                    ExpressionBlock intermediate_block =
                        render_expression(compiler, operand.expr, as_variable[j]);
                    write_type_info_to_section(
                        &block.section, intermediate_block.final_type
                    );
                    write_to_section(&block.section, as_variable[j]);
                    write_to_section(&block.section, ";\n");
                    write_to_section(&block.section, intermediate_block.section.buffer);
                    operand_types[j] = intermediate_block.final_type;
                    expression_block_free(intermediate_block);
                    break;
                }
                case OPERAND_TOKEN:
                    operand_types[j] = resolve_operand_type(&compiler->tc, operand);
                    break;
                default:
                    break;
            }
        }
        resolved_operation_types[i] =
            resolve_operation_type(operand_types[0], operand_types[1], operation.op_type);
        write_type_info_to_section(&block.section, resolved_operation_types[i]);
        write_to_section(&block.section, intermediate_variables[i]);
        write_to_section(&block.section, " = ");

        if (!is_unary) {
            size_t* left_ref = operand_to_resolved_operations[operation.left];
            if (left_ref) {
                write_to_section(&block.section, intermediate_variables[*left_ref]);
                *left_ref = i;
            }
            else {
                if (as_variable[0][0] != '\0') {
                    write_to_section(&block.section, as_variable[0]);
                }
                else {
                    write_simple_operand_to_section(
                        compiler, &block.section, expr->operands[operation.left]
                    );
                }
                size_t* new_ref = size_t_values + operation.left;
                *new_ref = i;
                operand_to_resolved_operations[operation.left] = new_ref;
            }
        }

        // TODO: this won't work for all operators
        write_to_section(&block.section, (char*)op_to_cstr(operation.op_type));

        size_t* right_ref = operand_to_resolved_operations[operation.right];
        if (right_ref) {
            write_to_section(&block.section, intermediate_variables[*right_ref]);
            *right_ref = i;
        }
        else {
            if (as_variable[1][0] != '\0') {
                write_to_section(&block.section, as_variable[1]);
            }
            else {
                write_simple_operand_to_section(
                    compiler, &block.section, expr->operands[operation.right]
                );
            }
            size_t* new_ref = size_t_values + operation.right;
            *new_ref = i;
            operand_to_resolved_operations[operation.right] = new_ref;
        }

        write_to_section(&block.section, ";\n");
    }

    write_to_section(&block.section, destination);
    write_to_section(&block.section, " = ");
    write_to_section(&block.section, intermediate_variables[expr->operations_count - 1]);
    write_to_section(&block.section, ";\n}\n");

    block.final_type = resolved_operation_types[expr->operations_count - 1];

    return block;
}

static void
compile_function(C_Compiler* compiler, FunctionStatement* func)
{
    write_type_info_to_section(&compiler->function_declarations, func->sig.return_type);
    write_to_section(&compiler->function_declarations, func->name);
    write_to_section(&compiler->function_declarations, "(");
    for (size_t i = 0; i < func->sig.params_count; i++) {
        if (i > 0) write_to_section(&compiler->function_declarations, ", ");
        write_type_info_to_section(&compiler->function_declarations, func->sig.types[i]);
        write_to_section(&compiler->function_declarations, func->sig.params[i]);
    }
    write_to_section(&compiler->function_declarations, ");\n");
    // TODO: function definition
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
compile_assignment(C_Compiler* compiler, AssignmentStatement* assignment)
{
    if (assignment->op_type != OPERATOR_ASSIGNMENT) {
        UNIMPLEMENTED("assignment op_type not implemented");
    }
    if (scope_stack_peek(&compiler->scope_stack) != compiler->top_level_scope)
        UNIMPLEMENTED("inner scope assignments not implemented");
    if (!is_simple_expression(assignment->storage)) {
        UNIMPLEMENTED("assignment to non-variables not yet implemented");
    }

    char* identifier = assignment->storage->operands[0].token.value;

    if (is_simple_expression(assignment->value)) {
        TypeInfo type_info =
            resolve_operand_type(&compiler->tc, assignment->value->operands[0]);
        declare_global_variable(compiler, type_info, identifier);
        write_to_section(&compiler->init_module_function, identifier);
        write_to_section(&compiler->init_module_function, " = ");
        write_simple_operand_to_section(
            compiler, &compiler->init_module_function, assignment->value->operands[0]
        );
        write_to_section(&compiler->init_module_function, ";\n");
    }
    else {
        ExpressionBlock value_render =
            render_expression(compiler, assignment->value, identifier);
        declare_global_variable(compiler, value_render.final_type, identifier);
        write_to_section(&compiler->init_module_function, value_render.section.buffer);
        expression_block_free(value_render);
    }
}

static void
compile_annotation(C_Compiler* compiler, AnnotationStatement* annotation)
{
    // TODO: make sure this isnt a re-declaration
    // TODO: implement default values for class members
    //
    if (scope_stack_peek(&compiler->scope_stack) != compiler->top_level_scope) return;

    declare_global_variable(compiler, annotation->type, annotation->identifier);

    if (annotation->initial) {
        if (is_simple_expression(annotation->initial)) {
            Operand operand = annotation->initial->operands[0];
            TypeInfo type_info =
                resolve_operand_type(&compiler->tc, annotation->initial->operands[0]);
            if (!compare_types(type_info, annotation->type))
                // TODO: implement a typechecker fucntion to determine if casting is safe
                UNIMPLEMENTED("assignments from differing types not implemented");
            write_to_section(&compiler->init_module_function, annotation->identifier);
            write_to_section(&compiler->init_module_function, " = ");
            write_simple_operand_to_section(
                compiler, &compiler->init_module_function, operand
            );
            write_to_section(&compiler->init_module_function, ";\n");
        }
        else {
            ExpressionBlock value_render =
                render_expression(compiler, annotation->initial, annotation->identifier);
            declare_global_variable(
                compiler, value_render.final_type, annotation->identifier
            );
            write_to_section(
                &compiler->init_module_function, value_render.section.buffer
            );
            expression_block_free(value_render);
        }
    }
}

void
compile_statement(C_Compiler* compiler, Statement* stmt)
{
    (void)compiler;
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
        case STMT_ASSIGNMENT:
            compile_assignment(compiler, stmt->assignment);
            break;
        case STMT_ANNOTATION:
            compile_annotation(compiler, stmt->annotation);
            break;
        case STMT_EXPR:
            UNIMPLEMENTED("expr statement is unimplemented");
        case STMT_NO_OP:
            UNIMPLEMENTED("no-op statement is unimplemented");
        case STMT_EOF:
            break;
        case STMT_RETURN:
            UNIMPLEMENTED("return statement is unimplemented");
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
    write_to_section(&compiler->main_function, "}");
    write_section_to_output(&compiler->forward, out);
    write_section_to_output(&compiler->struct_declarations, out);
    write_section_to_output(&compiler->variable_declarations, out);
    write_section_to_output(&compiler->function_declarations, out);
    write_section_to_output(&compiler->init_module_function, out);
    write_section_to_output(&compiler->main_function, out);
    fflush(out);
}

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

static C_Compiler
compiler_init(Lexer* lexer)
{
    TypeChecker tc = {.arena = lexer->arena, .globals = lexer->top_level, .locals = NULL};
    C_Compiler compiler = {.top_level_scope = lexer->top_level, .tc = tc};
    scope_stack_push(&compiler.scope_stack, lexer->top_level);
#if DEBUG
    write_debug_comment_breaks(&compiler);
#endif
    write_to_section(&compiler.forward, "#include <not_python.h>\n");
    write_to_section(&compiler.init_module_function, "static void init_module(void) {\n");
    write_to_section(&compiler.main_function, "int main(void) {\n");
    return compiler;
}

void
compile_to_c(FILE* outfile, Lexer* lexer)
{
    C_Compiler compiler = compiler_init(lexer);
    for (size_t i = 0; i < lexer->n_statements; i++) {
        compile_statement(&compiler, lexer->statements[i]);
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
