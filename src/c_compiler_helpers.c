#include "c_compiler_helpers.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
            return info.class_name;
        case PYTYPE_BOOL:
            return "bool";
        case PYTYPE_SLICE:
            return "slice";
        case PYTYPE_ITER:
            return "Iterator";
        case PYTYPE_DICT_ITEMS:
            return "DictItems";
        default:
            UNREACHABLE("default case type info to human readable");
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
    if (info.inner) {
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
type_info_to_c_syntax(TypeInfo info)
{
    switch (info.type) {
        case PYTYPE_UNTYPED:
            UNTYPED_ERROR();
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

void
write_type_info_to_section(CompilerSection* section, TypeInfo info)
{
    switch (info.type) {
        case PYTYPE_UNTYPED:
            UNTYPED_ERROR();
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
out_of_memory(void)
{
    fprintf(stderr, "ERROR: out of memory\n");
    exit(1);
}

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

size_t
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

int
str_hm_put_cstr(StringHashmap* hm, char* element)
{
    StringView sv = {.data = element, .offset = 0, .length = strlen(element)};
    return str_hm_put(hm, sv);
}

void
str_hm_free(StringHashmap* hm)
{
    free(hm->elements);
    free(hm->lookup);
}

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

bool
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

void
id_set_free(IdentifierSet* set)
{
    free(set->elements);
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

char*
sb_build(StringBuilder* sb, ...)
{
    va_list strings;
    va_start(strings, sb);

    char* start = sb->write;
    size_t initial_remaining = sb->remaining;
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
        }
    }

    *sb->write++ = '\0';
    sb->remaining -= 1;
    if (sb->remaining == 0) sb_start_new_buffer(sb);

    va_end(strings);

    return start;
}
