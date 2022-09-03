#include "c_compiler.h"

#include <assert.h>

#define UNIMPLEMENTED(msg) assert(0 && msg);
#define UNREACHABLE(msg) assert(0 && msg);

// TODO: some standard way to implement name mangling will be needed at some point

static void
out_of_memory(void)
{
    fprintf(stderr, "ERROR: out of memory\n");
    exit(1);
}

#define SECTION_CHUNK_SIZE 4096

typedef struct {
    size_t capacity;
    size_t remaining;
    char* buffer;
    char* write;
} CompilerSection;

static void
section_grow(CompilerSection* section)
{
    section->capacity += SECTION_CHUNK_SIZE;
    section->remaining += SECTION_CHUNK_SIZE;
    section->buffer = realloc(section->buffer, section->capacity);
    section->write = section->buffer + section->capacity - section->remaining;
    if (!section->buffer) out_of_memory();
}

static void
write_to_section(CompilerSection* section, char* data)
{
    char c;
    while ((c = *(data++)) != '\0') {
        if (section->remaining == 0) section_grow(section);
        *(section->write++) = c;
        section->remaining -= 1;
    }
}

typedef struct {
    CompilerSection functions;
    CompilerSection structs;
    CompilerSection vars;
    CompilerSection main;
    CompilerSection top_level_init;
} C_Compiler;

#define CDT_PYINT "long long int"
#define CDT_PYFLOAT "double"

static void
untyped_error(void)
{
    // TODO: locations of tokens should be available here to give a reasonable error
    // should it occur
    fprintf(stderr, "encountered untyped variable\n");
    exit(1);
}

static char*
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
            return CDT_PYINT " ";
        case PYTYPE_FLOAT:
            return CDT_PYFLOAT " ";
        case PYTYPE_STRING:
            UNIMPLEMENTED("str to c syntax unimplemented");
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
    UNREACHABLE("end of compile statement switch")
}

static void
write_function_declaration(C_Compiler* compiler, FunctionStatement* func)
{
    write_to_section(&compiler->functions, type_info_to_c_syntax(func->sig.return_type));
    write_to_section(&compiler->functions, func->name);
    write_to_section(&compiler->functions, "(");
    for (size_t i = 0; i < func->sig.params_count; i++) {
        if (i > 0) write_to_section(&compiler->functions, ", ");
        write_to_section(&compiler->functions, type_info_to_c_syntax(func->sig.types[i]));
        write_to_section(&compiler->functions, func->sig.params[i]);
    }
    write_to_section(&compiler->functions, ")");
}

static void
forward_declare_class(C_Compiler* compiler, ClassStatement* cls)
{
    write_to_section(&compiler->structs, "struct ");
    // TODO: name mangling
    write_to_section(&compiler->structs, cls->name);
    write_to_section(&compiler->structs, "{");
    size_t member_count = 0;
    for (size_t i = 0; i < cls->scope->hm.elements_count; i++) {
        Symbol sym = cls->scope->hm.elements[i];
        switch (sym.kind) {
            case SYM_FUNCTION:
                // TODO: requires some kind of name mangling
                write_function_declaration(compiler, sym.func);
                write_to_section(&compiler->functions, ";\n");
                break;
            case SYM_VARIABLE:
                // type objects don't currently exist
                break;
            case SYM_MEMBER:
                if (member_count > 0) write_to_section(&compiler->structs, ", ");
                write_to_section(
                    &compiler->structs, type_info_to_c_syntax(sym.member->type)
                );
                write_to_section(&compiler->structs, sym.member->identifier);
                member_count += 1;
                break;
            case SYM_CLASS:
                UNREACHABLE("nested classes should error in the lexer");
            default:
                UNREACHABLE("forward_declare_class default case unreachable")
        }
    }
    write_to_section(&compiler->structs, "};");
    if (member_count == 0) {
        // TODO: better error reporting or perhaps this should be an error from the parser
        // also worth considering if classes can exist without members as namespaces for
        // functions
        fprintf(stderr, "ERROR: class defined with no members\n");
        exit(1);
    }
}

static void
make_forward_declarations(C_Compiler* compiler, LexicalScope* top_level)
{
    for (size_t i = 0; i < top_level->hm.elements_count; i++) {
        Symbol sym = top_level->hm.elements[i];
        switch (sym.kind) {
            case SYM_FUNCTION:
                write_function_declaration(compiler, sym.func);
                write_to_section(&compiler->functions, ";\n");
                break;
            case SYM_VARIABLE:
                write_to_section(
                    &compiler->vars, type_info_to_c_syntax(sym.variable->type)
                );
                write_to_section(&compiler->vars, sym.variable->identifier);
                write_to_section(&compiler->vars, ";\n");
                break;
            case SYM_CLASS:
                forward_declare_class(compiler, sym.cls);
                break;
            case SYM_MEMBER:
                UNREACHABLE("there should be no members in the top level scope");
        }
    }
}

void
compile_statement(C_Compiler* compiler, Statement stmt)
{
    (void)compiler;
    switch (stmt.kind) {
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
            UNIMPLEMENTED("class compilation is unimplemented");
        case STMT_FUNCTION:
            UNIMPLEMENTED("function compilation is unimplemented");
        case STMT_ASSIGNMENT:
            UNIMPLEMENTED("assignment compilation is unimplemented");
        case STMT_ANNOTATION:
            UNIMPLEMENTED("annotation statement is unimplemented")
        case STMT_EXPR:
            UNIMPLEMENTED("annotation statement is unimplemented")
        case STMT_NO_OP:
            UNIMPLEMENTED("no-op statement is unimplemented")
        case STMT_EOF:
            UNIMPLEMENTED("no-op statement is unimplemented")
        case NULL_STMT:
            UNIMPLEMENTED("no-op statement is unimplemented")
        default:
            UNREACHABLE("default case unreachable")
    }
}

void
write_to_output(C_Compiler* compiler, FILE* out)
{
    // TODO: incomplete
    write_to_section(&compiler->structs, "\n");
    write_to_section(&compiler->vars, "\n");
    write_to_section(&compiler->functions, "\n");
    fwrite(
        compiler->structs.buffer,
        compiler->structs.capacity - compiler->structs.remaining,
        1,
        out
    );
    fwrite(
        compiler->vars.buffer, compiler->vars.capacity - compiler->vars.remaining, 1, out
    );
    fwrite(
        compiler->functions.buffer,
        compiler->functions.capacity - compiler->functions.remaining,
        1,
        out
    );
    fflush(out);
}

void
compile_to_c(FILE* outfile, Lexer* lexer)
{
    C_Compiler compiler = {0};
    make_forward_declarations(&compiler, lexer->top_level);
    // for (size_t i = 0; i < lexer->n_statements; i++) {
    // compile_statement(&compiler, lexer->statements[i]);
    // }
    write_to_output(&compiler, outfile);
}
