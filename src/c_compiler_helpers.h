#ifndef C_COMPILER_HELPERS_H
#define C_COMPILER_HELPERS_H

#include "not_python.h"

// TODO: some tests for various helper functions
//
#define UNREACHABLE(msg) assert(0 && msg);

typedef struct {
    size_t capacity;
    size_t count;
    StringView* elements;
    int* lookup;
} StringHashmap;

size_t str_hm_put(StringHashmap* hm, StringView element);
int str_hm_put_cstr(StringHashmap* hm, char* element);
void str_hm_free(StringHashmap* hm);

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

#endif
