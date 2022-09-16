#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include <stdarg.h>

#include "arena.h"
#include "tokens.h"

typedef struct {
    Arena* arena;
    const char* filepath;
    size_t bytes;
    size_t line_count;
    size_t line_capacity;
    size_t* line_offsets;
} FileIndex;

FileIndex create_file_index(Arena* arena, const char* filepath);
void file_index_index_line(FileIndex* index, size_t line_start);

void error(char* msg);
void errorf(char* fmt, ...);
void warn(char* msg);
void warnf(char* fmt, ...);

// ctx > 0 will render (ctx) surrounding lines
void syntax_error(FileIndex index, Location loc, size_t ctx, char* msg);
void syntax_errorf(FileIndex index, Location loc, size_t ctx, char* fmt, ...);

#endif
