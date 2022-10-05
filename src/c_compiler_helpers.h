#ifndef C_COMPILER_HELPERS_H
#define C_COMPILER_HELPERS_H

#include "diagnostics.h"
#include "not_python.h"
#include "syntax.h"

// TODO: some tests for various helper functions

// this is indicitive of a compiler bug, and TypeError from the user
// would come about when there are conflicting types, not missing types
#define UNTYPED_ERROR() assert(0 && "COMPILER ERROR: ran into untyped variable")

#define DATATYPE_INT "NpInt"
#define DATATYPE_FLOAT "NpFloat"
#define DATATYPE_STRING "NpString"
#define DATATYPE_BOOL "NpBool"
#define DATATYPE_LIST "NpList*"
#define DATATYPE_DICT "NpDict*"
#define DATATYPE_ITER "NpIter"
#define DATATYPE_FUNC "NpFunction"

SourceString create_default_object_fmt_str(Arena* arena, ClassStatement* clsdef);

typedef struct {
    size_t capacity;
    size_t count;
    SourceString* elements;
    int* lookup;
} StringHashmap;

size_t str_hm_put(StringHashmap* hm, SourceString element);
int str_hm_put_cstr(StringHashmap* hm, char* element);
void str_hm_free(StringHashmap* hm);

typedef struct {
    size_t capacity;
    size_t remaining;
    char* buffer;
    char* write;
} CompilerSection;

void section_free(CompilerSection* section);
void write_to_section(CompilerSection* section, char* data);
// LAST ... arg should be NULL
void write_many_to_section(CompilerSection* section, ...);
void copy_section(CompilerSection* dest, CompilerSection src);

/*
 * Begin a scope in which the given `section_ptr` is replaced with a temporary
 * CompilerSection which is then copied into the real section when the scope ends
 *
 * USAGE:
 * CompilerSection* my_current_section = ...;
 *
 * temporary_section(my_current_section) {
 *     // `my_current_section` is now a temporary section that will be copied to the real
 *     // section at the end of this scope
 *     ...
 * }
 */
#define temporary_section(section_ptr)                                                   \
    CompilerSection tmp##__LINE__ = {0};                                                 \
    CompilerSection* actual##__LINE__ = section_ptr;                                     \
    section_ptr = &tmp##__LINE__;                                                        \
    for (int i = 0; i < 1; i++,                                                          \
             copy_section(actual##__LINE__, tmp##__LINE__),                              \
             section_ptr = actual##__LINE__)

#define STRING_BUILDER_BUFFER_SIZE 4096

typedef struct {
    size_t remaining;
    char* write;
    size_t buffers_count;
    char** buffers;
} StringBuilder;

StringBuilder sb_init();
void sb_free(StringBuilder* sb);
SourceString sb_build(StringBuilder* sb, ...);
SourceString sb_join_ss(
    StringBuilder* sb, SourceString* strs, size_t count, const char* delimiter
);
const char* sb_c_cast(StringBuilder* sb, const char* cast_this, TypeInfo cast_to);

#define sb_build_cstr(sb, ...) sb_build(sb, __VA_ARGS__).data

void render_type_info_human_readable(TypeInfo info, char* buf, size_t buflen);
SourceString type_info_to_c_syntax_ss(StringBuilder* sb, TypeInfo info);
#define type_info_to_c_syntax(sb, type_info) type_info_to_c_syntax_ss(sb, type_info).data
void write_type_info_to_section(
    CompilerSection* section, StringBuilder* sb, TypeInfo info
);

const char* sort_cmp_for_type_info(TypeInfo type_info, bool reversed);
const char* voidptr_cmp_for_type_info(TypeInfo type_info);
const char* cmp_for_type_info(TypeInfo type_info);

const char* source_string_to_exception_type_string(
    FileIndex index, Location loc, SourceString str
);

#endif
