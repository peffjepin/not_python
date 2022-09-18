#ifndef C_COMPILER_HELPERS_H
#define C_COMPILER_HELPERS_H

#include "not_python.h"
#include "syntax.h"

// TODO: some tests for various helper functions

#define UNREACHABLE(msg) assert(0 && msg);
#define UNIMPLEMENTED(msg) assert(0 && msg)

// this is indicitive of a compiler bug, and TypeError from the user
// would come about when there are conflicting types, not missing types
#define UNTYPED_ERROR() assert(0 && "COMPILER ERROR: ran into untyped variable")

#define DATATYPE_INT "PYINT"
#define DATATYPE_FLOAT "PYFLOAT"
#define DATATYPE_STRING "PYSTRING"
#define DATATYPE_BOOL "PYBOOL"
#define DATATYPE_LIST "PYLIST"
#define DATATYPE_DICT "PYDICT"
#define DATATYPE_ITER "PYITER"

typedef struct {
    size_t capacity;
    size_t count;
    StringView* elements;
    int* lookup;
} StringHashmap;

size_t str_hm_put(StringHashmap* hm, StringView element);
int str_hm_put_cstr(StringHashmap* hm, char* element);
void str_hm_free(StringHashmap* hm);

// TODO: I think this is dead code now, try to remove.
typedef struct {
    size_t capacity;
    size_t count;
    char** elements;
} IdentifierSet;

// returns true if it added a new element or false if element already exists
bool id_set_add(IdentifierSet* set, char* id);
void id_set_free(IdentifierSet* set);

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

#define STRING_BUILDER_BUFFER_SIZE 4096

typedef struct {
    size_t remaining;
    char* write;
    size_t buffers_count;
    char** buffers;
} StringBuilder;

StringBuilder sb_init();
void sb_free(StringBuilder* sb);
char* sb_build(StringBuilder* sb, ...);

void render_type_info_human_readable(TypeInfo info, char* buf, size_t buflen);
const char* type_info_to_c_syntax(StringBuilder* sb, TypeInfo info);
void write_type_info_to_section(
    CompilerSection* section, TypeInfo info, StringBuilder* sb
);

#endif
