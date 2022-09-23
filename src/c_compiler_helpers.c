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
        case PYTYPE_NONE:
            return "None";
        case PYTYPE_INT:
            return "int";
        case PYTYPE_FLOAT:
            return "float";
        case PYTYPE_STRING:
            return "str";
        case PYTYPE_LIST:
            return "List";
        case PYTYPE_TUPLE:
            return "Tuple";
        case PYTYPE_DICT:
            return "Dict";
        case PYTYPE_OBJECT:
            return info.cls->name.data;
        case PYTYPE_BOOL:
            return "bool";
        case PYTYPE_SLICE:
            return "slice";
        case PYTYPE_ITER:
            return "Iterator";
        case PYTYPE_DICT_ITEMS:
            return "DictItems";
        case PYTYPE_UNTYPED:
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
    if (info.type != PYTYPE_OBJECT && info.inner) {
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
            remaining -= write_type_info_into_buffer_human_readable(
                info.inner->types[i], buffer, remaining
            );
        }
        if (remaining > 1) {
            *buffer++ = ']';
            remaining--;
        }
    }
    return start - remaining;
}

void
render_type_info_human_readable(TypeInfo info, char* buf, size_t buflen)
{
    size_t written = write_type_info_into_buffer_human_readable(info, buf, buflen);
    buf[written] = '\0';
}

const char*
type_info_to_c_syntax(StringBuilder* sb, TypeInfo info)
{
    switch (info.type) {
        case PYTYPE_UNTYPED:
            UNTYPED_ERROR();
            break;
        case PYTYPE_NONE:
            return "void*";
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
        case PYTYPE_OBJECT: {
            return sb_build(sb, info.cls->ns_ident.data, "*", NULL).data;
        }
        case PYTYPE_ITER:
            return DATATYPE_ITER;
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
    hm->elements = realloc(hm->elements, sizeof(StringView) * hm->capacity);
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
