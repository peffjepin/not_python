#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#include "compiler.h"
#include "diagnostics.h"
#include "tokens.h"

#define STRING_CONSTANTS_TABLE_NAME "NOT_PYTHON_STRING_CONSTANTS"

#define DATATYPE_INT "NpInt"
#define DATATYPE_FLOAT "NpFloat"
#define DATATYPE_STRING "NpString"
#define DATATYPE_BOOL "NpBool"
#define DATATYPE_LIST "NpList*"
#define DATATYPE_DICT "NpDict*"
#define DATATYPE_ITER "NpIter"
#define DATATYPE_FUNC "NpFunction"
#define DATATYPE_CONTEXT "NpContext"
#define DATATYPE_EXCEPTION "Exception*"
#define DATATYPE_NONE "NpNone"
#define DATATYPE_POINTER "NpPointer"
#define DATATYPE_DICT_ITEMS "DictItem"
#define DATATYPE_CSTR "char*"
#define DATATYPE_UNSIGNED "NpUnsigned"

typedef enum {
    SEC_FORWARD,
    SEC_TYPEDEFS,
    SEC_DECLARATIONS,
    SEC_DEFS,
    SEC_INIT,
    SEC_MAIN,
    SEC_COUNT
} SectionID;

typedef struct {
    size_t capacity;
    size_t remaining;
    char* buffer;
    char* write;
} Section;

typedef struct {
    const char* current_loop_after_label;
    Section sections[SEC_COUNT];
    FILE* out;
} Writer;

static void section_free(Section* section);
static void write(Section* section, const char* data);
static void write_string_constants_table(StringHashmap strings, Section* forward);
static void write_instruction(Writer* writer, SectionID s, Instruction inst);

void
write_c_program(CompiledInstructions instructions, FILE* out)
{
    Writer writer = {.out = out};

#if DEBUG
    write(writer.sections + SEC_FORWARD, "\n// FORWARD COMPILER SECTION\n");
    write(writer.sections + SEC_TYPEDEFS, "\n// TYPEDEFS COMPILER SECTION\n");
    write(writer.sections + SEC_DECLARATIONS, "\n// DECLARATIONS COMPILER SECTION\n");
    write(writer.sections + SEC_DEFS, "\n// FUNCTION DEFINITIONS COMPILER SECTION\n");
    write(writer.sections + SEC_INIT, "\n// INIT MODULE FUNCTION COMPILER SECTION\n");
    write(writer.sections + SEC_MAIN, "\n// MAIN FUNCTION COMPILER SECTION\n");
#endif

    write(writer.sections + SEC_FORWARD, "#include <not_python.h>\n");
    write(writer.sections + SEC_INIT, "static void init_module(void) {\n");
    write(writer.sections + SEC_MAIN, "int main(void) {\ninit_module();\n");
    write_string_constants_table(
        instructions.str_constants, writer.sections + SEC_FORWARD
    );

    if (instructions.req.libs[LIB_MATH])
        write(writer.sections + SEC_FORWARD, "#include <math.h>\n");

    // TODO: write instructions
    for (size_t i = 0; i < instructions.seq.count; i++) {
        Instruction inst = instructions.seq.instructions[i];
        write_instruction(&writer, SEC_INIT, inst);
    }

    write(writer.sections + SEC_INIT, "}");
    write(writer.sections + SEC_MAIN, "return 0;\n}");

    for (SectionID s = 0; s < SEC_COUNT; s++) {
        Section sec = writer.sections[s];
        fwrite(sec.buffer, sec.capacity - sec.remaining, 1, out);
        section_free(&sec);
    }

    fflush(out);
}

static void
section_grow(Section* section)
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
    if (!section->buffer) error("out of memory");
    memset(section->write, 0, section->remaining);
}

static void
section_free(Section* section)
{
    free(section->buffer);
}

static void
write(Section* section, const char* data)
{
    char c;
    while ((c = *(data++)) != '\0') {
        if (section->remaining <= 1) section_grow(section);
        *(section->write++) = c;
        section->remaining -= 1;
    }
}

static void
write_many(Section* section, const char** null_terminated_string_list)
{
    for (;;) {
        const char* arg = *null_terminated_string_list++;
        if (!arg) break;
        write(section, arg);
    }
}

static void
write_type_info(Section* section, TypeInfo info)
{
    switch (info.type) {
        case NPTYPE_UNTYPED:
            error("trying to write untyped variable to section");
            break;
        case NPTYPE_DICT_ITEMS:
            write(section, DATATYPE_DICT_ITEMS);
            break;
        case NPTYPE_POINTER:
            write(section, DATATYPE_POINTER);
            break;
        case NPTYPE_NONE:
            write(section, DATATYPE_NONE);
            break;
        case NPTYPE_INT:
            write(section, DATATYPE_INT);
            break;
        case NPTYPE_FLOAT:
            write(section, DATATYPE_FLOAT);
            break;
        case NPTYPE_STRING:
            write(section, DATATYPE_STRING);
            break;
        case NPTYPE_BOOL:
            write(section, DATATYPE_BOOL);
            break;
        case NPTYPE_LIST:
            write(section, DATATYPE_LIST);
            break;
        case NPTYPE_TUPLE:
            UNIMPLEMENTED("tuple to c syntax unimplemented");
            break;
        case NPTYPE_DICT:
            write(section, DATATYPE_DICT);
            break;
        case NPTYPE_OBJECT:
            write_many(section, (const char* [3]){info.cls->ns_ident.data, "*", NULL});
            break;
        case NPTYPE_FUNCTION:
            write(section, DATATYPE_FUNC);
            break;
        case NPTYPE_ITER:
            write(section, DATATYPE_ITER);
            break;
        case NPTYPE_CONTEXT:
            write(section, DATATYPE_CONTEXT);
            break;
        case NPTYPE_EXCEPTION:
            write(section, DATATYPE_EXCEPTION);
            break;
        case NPTYPE_CSTR:
            write(section, DATATYPE_CSTR);
            break;
        case NPTYPE_UNSIGNED:
            write(section, DATATYPE_UNSIGNED);
            break;
        default: {
            errorf("no C syntax implementation for type `%s`", errfmt_type_info(info));
            UNREACHABLE();
        }
    }
}

static void
write_string_constants_table(StringHashmap strings, Section* forward)
{
    write(forward, DATATYPE_STRING " " STRING_CONSTANTS_TABLE_NAME "[] = {\n");
    for (size_t i = 0; i < strings.count; i++) {
        SourceString str = strings.elements[i];
        if (i > 0) write(forward, ",\n");
        write(forward, "{.data=\"");
        write(forward, str.data);
        write(forward, "\", .length=");
        char length_as_str[21];
        snprintf(length_as_str, 21, "%zu", str.length);
        write(forward, length_as_str);
        write(forward, "}");
    }
    write(forward, "};\n");
}

static void
write_ident(Section* section, StorageIdent ident)
{
    const int buflen = 128;
    char buffer[buflen];

    if (ident.reference) write(section, "&");

    switch (ident.kind) {
        case IDENT_CSTR:
            write(section, ident.cstr);
            break;
        case IDENT_VAR:
            write(section, ident.var->compiled_name.data);
            break;
        case IDENT_INT_LITERAL:
            if (snprintf(buffer, buflen, "%i", ident.int_value) >= buflen)
                error("int literal buffer overflow");
            write(section, buffer);
            break;
        case IDENT_FLOAT_LITERAL:
            if (snprintf(buffer, buflen, "%f", ident.float_value) >= buflen)
                error("float literal buffer overflow");
            write(section, buffer);
            break;
        case IDENT_STRING_LITERAL:
            if (snprintf(buffer, buflen, "%zu", ident.str_literal_index) >= buflen)
                error("string literal index overflow");
            write_many(
                section,
                (const char*[]){STRING_CONSTANTS_TABLE_NAME, "[", buffer, "]", NULL}
            );
            break;
    }
}

static void
write_casted_ident(Section* section, TypeInfo cast, StorageIdent ident)
{
    write(section, "((");
    write_type_info(section, cast);
    write(section, ")");
    write_ident(section, ident);
    write(section, ")");
}

static void
write_intrinsic(Section* section, StorageIdent left, Operator op, StorageIdent right)
{
    switch (op) {
        case OPERATOR_PLUS:
            write_ident(section, left);
            write(section, " + ");
            write_ident(section, right);
            break;
        case OPERATOR_MINUS:
            write_ident(section, left);
            write(section, " - ");
            write_ident(section, right);
            break;
        case OPERATOR_MULT:
            write_ident(section, left);
            write(section, " * ");
            write_ident(section, right);
            break;
        case OPERATOR_DIV:
            write_casted_ident(section, FLOAT_TYPE, left);
            write(section, " / ");
            write_casted_ident(section, FLOAT_TYPE, right);
            break;
        case OPERATOR_MOD:
            write_ident(section, left);
            write(section, " % ");
            write_ident(section, right);
            break;
        case OPERATOR_FLOORDIV:
            write(section, "(" DATATYPE_INT ")(");
            write_casted_ident(section, FLOAT_TYPE, left);
            write(section, " / ");
            write_casted_ident(section, FLOAT_TYPE, right);
            write(section, ")");
            break;
        case OPERATOR_EQUAL:
            write_ident(section, left);
            write(section, " == ");
            write_ident(section, right);
            break;
        case OPERATOR_NOT_EQUAL:
            write_ident(section, left);
            write(section, " != ");
            write_ident(section, right);
            break;
        case OPERATOR_GREATER:
            write_ident(section, left);
            write(section, " > ");
            write_ident(section, right);
            break;
        case OPERATOR_LESS:
            write_ident(section, left);
            write(section, " < ");
            write_ident(section, right);
            break;
        case OPERATOR_GREATER_EQUAL:
            write_ident(section, left);
            write(section, " >= ");
            write_ident(section, right);
            break;
        case OPERATOR_LESS_EQUAL:
            write_ident(section, left);
            write(section, " <= ");
            write_ident(section, right);
            break;
        case OPERATOR_BITWISE_AND:
            write_ident(section, left);
            write(section, " & ");
            write_ident(section, right);
            break;
        case OPERATOR_BITWISE_OR:
            write_ident(section, left);
            write(section, " | ");
            write_ident(section, right);
            break;
        case OPERATOR_BITWISE_XOR:
            write_ident(section, left);
            write(section, " ^ ");
            write_ident(section, right);
            break;
        case OPERATOR_LSHIFT:
            write_ident(section, left);
            write(section, " << ");
            write_ident(section, right);
            break;
        case OPERATOR_RSHIFT:
            write_ident(section, left);
            write(section, " >> ");
            write_ident(section, right);
            break;
        case OPERATOR_LOGICAL_AND:
            write_ident(section, left);
            write(section, " && ");
            write_ident(section, right);
            break;
        case OPERATOR_LOGICAL_OR:
            write_ident(section, left);
            write(section, " || ");
            write_ident(section, right);
            break;
        case OPERATOR_LOGICAL_NOT:
            write(section, "!");
            write_ident(section, right);
            break;
        case OPERATOR_IS:
            write_ident(section, left);
            write(section, " == ");
            write_ident(section, right);
            break;
        case OPERATOR_NEGATIVE:
            write(section, "-");
            write_ident(section, right);
            break;
        case OPERATOR_BITWISE_NOT:
            write(section, "~");
            write_ident(section, right);
            break;
        default:
            errorf(
                "unexpected instrinsic: %s %s %s",
                errfmt_type_info(left.info),
                op_to_cstr(op),
                errfmt_type_info(right.info)
            );
    }
}

static void
write_ident_attr(Section* section, StorageIdent object, const char* attr)
{
    // TODO: test error message on bad attribute access

    switch (object.info.type) {
        case NPTYPE_STRING:
            write_ident(section, object);
            write_many(section, (const char*[]){".", attr, NULL});
            break;
        case NPTYPE_LIST:
            write_ident(section, object);
            write_many(section, (const char*[]){"->", attr, NULL});
            break;
        case NPTYPE_DICT:
            write_ident(section, object);
            write_many(section, (const char*[]){"->", attr, NULL});
            break;
        case NPTYPE_OBJECT:
            write_ident(section, object);
            write_many(section, (const char*[]){"->", attr, NULL});
            break;
        case NPTYPE_ITER:
            write_ident(section, object);
            write_many(section, (const char*[]){".", attr, NULL});
            break;
        case NPTYPE_FUNCTION:
            write_ident(section, object);
            write_many(section, (const char*[]){".", attr, NULL});
            break;
        case NPTYPE_DICT_ITEMS:
            write_ident(section, object);
            write_many(section, (const char*[]){".", attr, NULL});
            break;
        case NPTYPE_CONTEXT:
            write_ident(section, object);
            write_many(section, (const char*[]){".", attr, NULL});
            break;
        case NPTYPE_EXCEPTION:
            write_ident(section, object);
            write_many(section, (const char*[]){"->", attr, NULL});
            break;
        case NPTYPE_POINTER:
            write_ident(section, object);
            write_many(section, (const char*[]){"->", attr, NULL});
            break;
        default:
            errorf(
                "unexpected attribute access: object type = %s, attr = %s",
                errfmt_type_info(object.info),
                attr
            );
    }
}

static void
write_operation(Section* section, OperationInst operation_inst)
{
    switch (operation_inst.kind) {
        case OPERATION_INTRINSIC:
            write_intrinsic(
                section, operation_inst.left, operation_inst.op, operation_inst.right
            );
            break;
        case OPERATION_FUNCTION_CALL:
            // cast func.addr to correct type -> ((cast_type)function.addr)
            // cast type -> return_type (*) (NpContext, param_type_0, ...)
            write(section, "((");
            write_type_info(section, operation_inst.function.info.sig->return_type);
            write(section, " (*)(" DATATYPE_CONTEXT);
            for (size_t i = 0; i < operation_inst.function.info.sig->params_count; i++) {
                write(section, ", ");
                write_type_info(section, operation_inst.function.info.sig->types[i]);
            }
            write(section, "))");
            write_ident(section, operation_inst.function);
            write(section, ".addr)");

            // write call -> (function.ctx, ...)
            write(section, "(");
            write_ident(section, operation_inst.function);
            write(section, ".ctx");
            for (size_t i = 0; i < operation_inst.function.info.sig->params_count; i++) {
                write(section, ", ");
                write_ident(section, operation_inst.args[i]);
            }
            write(section, ")");
            break;
        case OPERATION_C_CALL:
            write_many(section, (const char*[]){operation_inst.c_fn_name, "(", NULL});
            for (size_t i = 0; i < operation_inst.c_fn_argc; i++) {
                if (i > 0) write(section, ", ");
                write_ident(section, operation_inst.c_fn_argv[i]);
            }
            write(section, ")");
            break;
        case OPERATION_C_CALL1:
            write_many(section, (const char*[]){operation_inst.c_fn_name, "(", NULL});
            write_ident(section, operation_inst.c_fn_arg);
            write(section, ")");
            break;
        case OPERATION_GET_ATTR:
            write_ident_attr(section, operation_inst.object, operation_inst.attr.data);
            break;
        case OPERATION_SET_ATTR:
            write_ident_attr(section, operation_inst.object, operation_inst.attr.data);
            write(section, " = ");
            write_ident(section, operation_inst.value);
            break;
        case OPERATION_COPY:
            write_ident(section, operation_inst.copy);
            break;
        case OPERATION_DEREF:
            // *((type*)ref)
            write(section, "*((");
            write_type_info(section, operation_inst.info);
            write(section, "*)");
            write_ident(section, operation_inst.ref);
            write(section, ")");
    }
}

static void
write_instruction(Writer* writer, SectionID s, Instruction inst)
{
    switch (inst.kind) {
        case INST_ITER_NEXT:
            // iter.next_data = iter.next(iter.iter);
            write_ident_attr(writer->sections + s, inst.iter_next.iter, "next_data");
            write(writer->sections + s, " = ");
            write_ident_attr(writer->sections + s, inst.iter_next.iter, "next");
            write(writer->sections + s, "(");
            write_ident_attr(writer->sections + s, inst.iter_next.iter, "iter");
            write(writer->sections + s, ");\n");

            // if (iter.next_data)
            //     unpack = *((iter_type*)iter.next_data);

            write(writer->sections + s, "if (");
            write_ident_attr(writer->sections + s, inst.iter_next.iter, "next_data");
            write(writer->sections + s, ") ");

            write_ident(writer->sections + s, inst.iter_next.unpack);
            write(writer->sections + s, " = *((");
            write_type_info(
                writer->sections + s, inst.iter_next.iter.info.inner->types[0]
            );
            write(writer->sections + s, "*)");
            write_ident_attr(writer->sections + s, inst.iter_next.iter, "next_data");
            write(writer->sections + s, ");\n");
            break;
        case INST_OPERATION:
            write_operation(writer->sections + s, inst.operation);
            write(writer->sections + s, ";\n");
            break;
        case INST_RETURN:
            write(writer->sections + s, "return ");
            write_ident(writer->sections + s, inst.rtval);
            write(writer->sections + s, ";\n");
            break;
        case INST_LABEL:
            write_many(writer->sections + s, (const char*[]){inst.label, ":\n", NULL});
            break;
        case INST_GOTO:
            write_many(
                writer->sections + s, (const char*[]){"goto ", inst.label, ";\n", NULL}
            );
            break;
        case INST_ELSE:
            for (size_t i = 0; i < inst.else_.count; i++)
                write_instruction(writer, s, inst.else_.instructions[i]);
            break;
        case INST_IF:
            write(writer->sections + s, "if (");
            if (inst.if_.negate) write(writer->sections + s, "!");
            write_ident(writer->sections + s, inst.if_.condition_ident);
            write(writer->sections + s, ") {\n");
            for (size_t i = 0; i < inst.if_.body.count; i++)
                write_instruction(writer, s, inst.if_.body.instructions[i]);
            write(writer->sections + s, "}\n");
            break;
        case INST_LOOP: {
            const char* current = writer->current_loop_after_label;
            writer->current_loop_after_label = inst.loop.after_label;

            // init
            for (size_t i = 0; i < inst.loop.init.count; i++)
                write_instruction(writer, s, inst.loop.init.instructions[i]);

            // loop
            write(writer->sections + s, "while(");
            write_ident(writer->sections + s, inst.loop.condition);
            write(writer->sections + s, ") {\n");

            // before
            for (size_t i = 0; i < inst.loop.before.count; i++)
                write_instruction(writer, s, inst.loop.before.instructions[i]);

            // body
            for (size_t i = 0; i < inst.loop.body.count; i++)
                write_instruction(writer, s, inst.loop.body.instructions[i]);

            // after
            for (size_t i = 0; i < inst.loop.after.count; i++)
                write_instruction(writer, s, inst.loop.after.instructions[i]);

            write(writer->sections + s, "}\n");
            writer->current_loop_after_label = current;
            break;
        }
        case INST_NO_OP:
            break;
        case INST_ASSIGNMENT:
            write_ident(writer->sections + s, inst.assignment.left);
            write(writer->sections + s, " = ");
            write_operation(writer->sections + s, inst.assignment.right);
            write(writer->sections + s, ";\n");
            break;
        case INST_DECLARE_VARIABLE: {
            bool is_variable = inst.declare_variable.kind == IDENT_VAR;
            Section* section = (s == SEC_INIT && is_variable)
                                   ? writer->sections + SEC_DECLARATIONS
                                   : writer->sections + s;

            write_type_info(
                section,
                (is_variable) ? inst.declare_variable.var->type_info
                              : inst.declare_variable.info
            );
            write(section, " ");
            write_ident(section, inst.declare_variable);
            write(section, ";\n");
            break;
        }
        case INST_DECL_ASSIGNMENT:
            write_instruction(
                writer,
                s,
                (Instruction){
                    .kind = INST_DECLARE_VARIABLE,
                    .declare_variable = inst.assignment.left,
                }
            );
            write_instruction(
                writer,
                s,
                (Instruction){
                    .kind = INST_ASSIGNMENT,
                    .assignment = inst.assignment,
                }
            );
            break;
        case INST_BREAK:
            write(writer->sections + s, "break;\n");
            break;
        case INST_CONTINUE:
            assert(writer->current_loop_after_label);
            write_many(
                writer->sections + s,
                (const char*[]){"goto ", writer->current_loop_after_label, ";\n", NULL}
            );
            break;
        case INST_DEFINE_CLASS:
            write(writer->sections + SEC_TYPEDEFS, "typedef struct { ");
            for (size_t i = 0; i < inst.define_class.signature.params_count; i++) {
                write_type_info(
                    writer->sections + SEC_TYPEDEFS, inst.define_class.signature.types[i]
                );
                write_many(
                    writer->sections + SEC_TYPEDEFS,
                    (const char*[]){
                        " ", inst.define_class.signature.params[i].data, "; ", NULL}
                );
            }
            write_many(
                writer->sections + SEC_TYPEDEFS,
                (const char*[]){"} ", inst.define_class.class_name, ";\n", NULL}
            );
            for (size_t i = 0; i < inst.define_class.body.count; i++) {
                Instruction body_inst = inst.define_class.body.instructions[i];
                write_instruction(writer, s, body_inst);
            }
            break;
        case INST_DEFINE_FUNCTION: {
            // `return_type function_name(NpContext __ctx__`
            write_type_info(
                writer->sections + SEC_DEFS, inst.define_function.signature.return_type
            );
            write_many(
                writer->sections + SEC_DEFS,
                (const char*[]){
                    " ",
                    inst.define_function.function_name,
                    "(" DATATYPE_CONTEXT " __ctx__",
                    NULL}
            );

            // `, type1 param1` for each param
            for (size_t i = 0; i < inst.define_function.signature.params_count; i++) {
                write(writer->sections + SEC_DEFS, ", ");
                write_type_info(
                    writer->sections + SEC_DEFS, inst.define_function.signature.types[i]
                );
                write_many(
                    writer->sections + SEC_DEFS,
                    (const char*[]){
                        " ", inst.define_function.signature.params[i].data, NULL}
                );
            }

            // begin scope -- function body -- end scope
            write(writer->sections + SEC_DEFS, ") {\n");
            for (size_t i = 0; i < inst.define_function.body.count; i++) {
                Instruction body_inst = inst.define_function.body.instructions[i];
                write_instruction(writer, SEC_DEFS, body_inst);
            }
            write(writer->sections + SEC_DEFS, "}\n");
        }
    }
}
