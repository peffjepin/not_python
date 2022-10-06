#include "c_compiler_helpers.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "diagnostics.h"
#include "np_hash.h"

static const char*
type_info_human_readable(TypeInfo info)
{
    switch (info.type) {
        case NPTYPE_NONE:
            return "None";
        case NPTYPE_INT:
            return "int";
        case NPTYPE_FLOAT:
            return "float";
        case NPTYPE_STRING:
            return "str";
        case NPTYPE_LIST:
            return "List";
        case NPTYPE_TUPLE:
            return "Tuple";
        case NPTYPE_DICT:
            return "Dict";
        case NPTYPE_FUNCTION:
            return "Function";
        case NPTYPE_OBJECT:
            return info.cls->name.data;
        case NPTYPE_BOOL:
            return "bool";
        case NPTYPE_SLICE:
            return "slice";
        case NPTYPE_ITER:
            return "Iter";
        case NPTYPE_DICT_ITEMS:
            return "DictItems";
        case NPTYPE_UNTYPED:
            return "ERROR: UNTYPED";
        default:
            UNREACHABLE();
    }
}

size_t
write_type_info_into_buffer_human_readable(TypeInfo info, char* buffer, size_t remaining)
{
    size_t start = remaining;
    const char* outer = type_info_human_readable(info);

    while (*outer != '\0' && remaining > 1) {
        *buffer++ = *outer++;
        remaining--;
    }
    if (info.type == NPTYPE_FUNCTION) {
        if (remaining > 2) {
            *buffer++ = '[';
            *buffer++ = '[';
            remaining -= 2;
        }
        for (size_t i = 0; i < info.sig->params_count; i++) {
            if (i > 0 && remaining > 2) {
                *buffer++ = ',';
                *buffer++ = ' ';
                remaining -= 2;
            }
            remaining -= write_type_info_into_buffer_human_readable(
                info.sig->types[i], buffer, remaining
            );
        }
        if (remaining > 2) {
            *buffer++ = ']';
            *buffer++ = ',';
            *buffer++ = ' ';
            remaining -= 3;
        }
        size_t written = write_type_info_into_buffer_human_readable(
            info.sig->return_type, buffer, remaining
        );
        remaining -= written;
        buffer += written;
        if (remaining > 0) {
            *buffer++ = ']';
            remaining -= 1;
        }
    }
    else if (info.type != NPTYPE_OBJECT && info.inner) {
        if (remaining > 1) {
            *buffer++ = '[';
            remaining--;
        }
        for (size_t i = 0; i < info.inner->count; i++) {
            if (i > 0 && remaining > 2) {
                *buffer++ = ',';
                *buffer++ = ' ';
                remaining -= 2;
            }
            size_t written = write_type_info_into_buffer_human_readable(
                info.inner->types[i], buffer, remaining
            );
            remaining -= written;
            buffer += written;
        }
        if (remaining > 1) {
            *buffer++ = ']';
            remaining--;
        }
    }
    *buffer = '\0';
    return start - remaining;
}

void
render_type_info_human_readable(TypeInfo info, char* buf, size_t buflen)
{
    size_t written = write_type_info_into_buffer_human_readable(info, buf, buflen);
    buf[written] = '\0';
}

const char*
type_infos_to_comma_separated_c_syntax(
    StringBuilder* sb, TypeInfo* types, size_t types_count
)
{
    SourceString c_syntax[types_count];
    for (size_t i = 0; i < types_count; i++) {
        c_syntax[i] = type_info_to_c_syntax_ss(sb, types[i]);
    }
    return sb_join_ss(sb, c_syntax, types_count, ", ").data;
}

SourceString
type_info_to_c_syntax_ss(StringBuilder* sb, TypeInfo info)
{
    switch (info.type) {
        case NPTYPE_UNTYPED:
            UNTYPED_ERROR();
            break;
        case NPTYPE_NONE: {
            static const SourceString NONE = {.data = "void*", .length = 4};
            return NONE;
        }
        case NPTYPE_INT: {
            static const SourceString INT = {
                .data = DATATYPE_INT, .length = sizeof(DATATYPE_INT) - 1};
            return INT;
        }
        case NPTYPE_FLOAT: {
            static const SourceString FLOAT = {
                .data = DATATYPE_FLOAT, .length = sizeof(DATATYPE_FLOAT) - 1};
            return FLOAT;
        }
        case NPTYPE_STRING: {
            static const SourceString STRING = {
                .data = DATATYPE_STRING, .length = sizeof(DATATYPE_STRING) - 1};
            return STRING;
        }
        case NPTYPE_BOOL: {
            static const SourceString BOOL = {
                .data = DATATYPE_BOOL, .length = sizeof(DATATYPE_BOOL) - 1};
            return BOOL;
        }
        case NPTYPE_LIST: {
            static const SourceString LIST = {
                .data = DATATYPE_LIST, .length = sizeof(DATATYPE_LIST) - 1};
            return LIST;
        }
        case NPTYPE_TUPLE:
            UNIMPLEMENTED("tuple to c syntax unimplemented");
            break;
        case NPTYPE_DICT: {
            static const SourceString DICT = {
                .data = DATATYPE_DICT, .length = sizeof(DATATYPE_DICT) - 1};
            return DICT;
        }
        case NPTYPE_OBJECT:
            return sb_build(sb, info.cls->ns_ident.data, "*", NULL);
        case NPTYPE_FUNCTION: {
            static const SourceString FUNC = {
                .data = DATATYPE_FUNC, .length = sizeof(DATATYPE_FUNC) - 1};
            return FUNC;
        }
        case NPTYPE_ITER: {
            static const SourceString ITER = {
                .data = DATATYPE_ITER, .length = sizeof(DATATYPE_ITER) - 1};
            return ITER;
        }
        default: {
            char buf[1024];
            render_type_info_human_readable(info, buf, 1024);
            fprintf(stderr, "%s\n", buf);
            fflush(stderr);
            UNREACHABLE();
            break;
        }
    }
    UNREACHABLE();
}

void
write_type_info_to_section(CompilerSection* section, StringBuilder* sb, TypeInfo info)
{
    write_to_section(section, (char*)type_info_to_c_syntax(sb, info));
    write_to_section(section, " ");
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
    memcpy(rtval.data, fmtbuf, rtval.length);
    return rtval;
}

static void
out_of_memory(void)
{
    fprintf(stderr, "ERROR: out of memory\n");
    exit(1);
}

static void
str_hm_rehash(StringHashmap* hm)
{
    for (size_t i = 0; i < hm->count; i++) {
        SourceString str = hm->elements[i];
        uint64_t hash = hash_bytes(str.data, str.length);
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
    if (!hm->elements || !hm->lookup) out_of_memory();
    memset(hm->lookup, -1, sizeof(int) * 2 * hm->capacity);
    str_hm_rehash(hm);
}

size_t
str_hm_put(StringHashmap* hm, SourceString element)
{
    if (hm->count == hm->capacity) str_hm_grow(hm);
    uint64_t hash = hash_bytes(element.data, element.length);
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

int
str_hm_put_cstr(StringHashmap* hm, char* element)
{
    SourceString str = {.data = element, .length = strlen(element)};
    return str_hm_put(hm, str);
}

void
str_hm_free(StringHashmap* hm)
{
    free(hm->elements);
    free(hm->lookup);
}

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

void
section_free(CompilerSection* section)
{
    free(section->buffer);
}

void
copy_section(CompilerSection* dest, CompilerSection src)
{
    size_t src_length = src.capacity - src.remaining;
    if (src_length == 0) return;
    if (!dest->write) dest->write = dest->buffer;

    int extra_space_required = src_length - dest->remaining;
    if (extra_space_required > 0) {
        ptrdiff_t write_off = dest->write - dest->buffer;
        dest->capacity = 2 * (dest->capacity + extra_space_required);
        size_t middle = dest->capacity / 2;
        dest->remaining += middle + extra_space_required;
        dest->buffer = realloc(dest->buffer, dest->capacity);
        if (!dest->buffer) out_of_memory();
        dest->write = dest->buffer + write_off;
        memset(dest->buffer + middle, 0, middle);
    }
    memcpy(dest->write, src.buffer, src_length);
    dest->write += src_length;
    dest->remaining -= src_length;
}

void
write_to_section(CompilerSection* section, char* data)
{
    char c;
    while ((c = *(data++)) != '\0') {
        if (section->remaining <= 1) section_grow(section);
        *(section->write++) = c;
        section->remaining -= 1;
    }
}

void
write_many_to_section(CompilerSection* section, ...)
{
    va_list vargs;
    va_start(vargs, section);

    for (;;) {
        char* arg = va_arg(vargs, char*);
        if (!arg) break;
        write_to_section(section, arg);
    }

    va_end(vargs);
}

#define STRING_BUILDER_BUFFER_SIZE 4096

static void
sb_start_new_buffer(StringBuilder* sb)
{
    sb->buffers_count++;
    sb->buffers = realloc(sb->buffers, sizeof(char*) * sb->buffers_count);
    if (!sb->buffers) out_of_memory();
    sb->buffers[sb->buffers_count - 1] = calloc(1, STRING_BUILDER_BUFFER_SIZE);
    if (!sb->buffers[sb->buffers_count - 1]) out_of_memory();
    sb->write = sb->buffers[sb->buffers_count - 1];
    sb->remaining = STRING_BUILDER_BUFFER_SIZE;
}

static void
sb_inherit_buffer(StringBuilder* sb, char* buf)
{
    sb->buffers_count++;
    sb->buffers = realloc(sb->buffers, sizeof(char*) * sb->buffers_count);
    if (!sb->buffers) out_of_memory();
    sb->buffers[sb->buffers_count - 1] = buf;
}

StringBuilder
sb_init()
{
    StringBuilder builder = {0};
    sb_start_new_buffer(&builder);
    return builder;
}

void
sb_free(StringBuilder* sb)
{
    for (size_t i = 0; i < sb->buffers_count; i++) free(sb->buffers[i]);
    free(sb->buffers);
}

SourceString
sb_build(StringBuilder* sb, ...)
{
    va_list strings;
    va_start(strings, sb);

    char* start = sb->write;
    size_t initial_remaining = sb->remaining;
    size_t length = 0;
    bool fresh_buffer = false;

    for (;;) {
        char* string = va_arg(strings, char*);
        if (!string) break;

        char c;
        while ((c = *string++) != '\0') {
            if (sb->remaining == 1) {
                if (fresh_buffer) {
                    fprintf(stderr, "ERROR: string builder overflow\n");
                    exit(1);
                }
                sb_start_new_buffer(sb);
                memcpy(sb->write, start, initial_remaining - 1);
                start = sb->write;
                sb->write += initial_remaining - 1;
                sb->remaining -= initial_remaining - 1;
                fresh_buffer = true;
            }
            *sb->write++ = c;
            sb->remaining -= 1;
            length++;
        }
    }

    *sb->write++ = '\0';
    sb->remaining -= 1;
    if (sb->remaining == 0) sb_start_new_buffer(sb);

    va_end(strings);

    return (SourceString){.data = start, .length = length};
}

SourceString
sb_join_ss(StringBuilder* sb, SourceString* strs, size_t count, const char* delimiter)
{
    if (count == 0) return (SourceString){.data = "", .length = 0};
    size_t delim_length = strlen(delimiter);
    size_t required = 1 + delim_length * (count - 1);
    for (size_t i = 0; i < count; i++) required += strs[i].length;

    char* buf;
    if (sb->remaining < required) {
        buf = malloc(required);
        if (!buf) out_of_memory();
        sb_inherit_buffer(sb, buf);
    }
    else {
        buf = sb->write;
        sb->remaining -= required;
        sb->write += required;
    }

    char* write = buf;
    for (size_t i = 0; i < count; i++) {
        if (i > 0) {
            memcpy(write, delimiter, delim_length);
            write += delim_length;
        }
        memcpy(write, strs[i].data, strs[i].length);
        write += strs[i].length;
    }
    *write = '\0';

    return (SourceString){.data = buf, .length = required};
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

const char*
source_string_to_exception_type_string(FileIndex index, Location loc, SourceString str)
{
    // TODO: the parser should probably do this
    static const SourceString MemoryError = {.data = "MemoryError", .length = 11};
    static const SourceString IndexError = {.data = "IndexError", .length = 10};
    static const SourceString KeyError = {.data = "KeyError", .length = 8};
    static const SourceString ValueError = {.data = "ValueError", .length = 10};
    static const SourceString AssertionError = {.data = "AssertionError", .length = 14};

    if (SOURCESTRING_EQ(str, MemoryError)) return "MEMORY_ERROR";
    if (SOURCESTRING_EQ(str, IndexError)) return "INDEX_ERROR";
    if (SOURCESTRING_EQ(str, ValueError)) return "VALUE_ERROR";
    if (SOURCESTRING_EQ(str, KeyError)) return "KEY_ERROR";
    if (SOURCESTRING_EQ(str, AssertionError)) return "ASSERTION_ERROR";
    unspecified_errorf(index, loc, "unrecognized exception type `%s`", str.data);
    UNREACHABLE();
}

const char*
sb_c_cast(StringBuilder* sb, const char* cast_this, TypeInfo cast_to)
{
    if (cast_to.type == NPTYPE_FUNCTION) {
        const char* params_rendered = type_infos_to_comma_separated_c_syntax(
            sb, cast_to.sig->types, cast_to.sig->params_count
        );
        const char* c_function_typing_syntax = sb_build_cstr(
            sb,
            type_info_to_c_syntax(sb, cast_to.sig->return_type),
            " (*)(NpContext ctx",
            (*params_rendered) ? ", " : "",
            params_rendered,
            ")",
            NULL
        );
        return sb_build_cstr(
            sb, "((", c_function_typing_syntax, ")", cast_this, ")", NULL
        );
    }
    return sb_build_cstr(
        sb, "((", type_info_to_c_syntax(sb, cast_to), ")", cast_this, ")", NULL
    );
}
