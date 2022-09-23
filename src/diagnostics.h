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

typedef enum {
    LABEL_SYNTAX_ERROR,
    LABEL_DEFAULT_ERROR,
    LABEL_TYPE_ERROR,
    LABEL_NAME_ERROR,
    LABEL_WARNING,
    LABEL_UNSPECIFIED_ERROR,
    LABEL_SOURCE_CODE,
    LABEL_SOURCE_CODE_HIGHLIGHTED,
    LABEL_NORMAL,
    LABEL_DEBUG,
    LABEL_UNREACHABLE,
    LABEL_UNIMPLEMENTED,
    LABEL_COUNT,
} DiagnosticLabel;

void diagnostic_vprintf(DiagnosticLabel label, char* fmt, va_list args, bool newline);
void diagnostic_printf(DiagnosticLabel label, char* fmt, ...);

void error(char* msg);
void errorf(char* fmt, ...);
void warn(char* msg);
void warnf(char* fmt, ...);
void debug(char* msg);
void debugf(char* fmt, ...);

// ctx > 0 will render (ctx) surrounding lines
void syntax_error(FileIndex index, Location loc, size_t ctx, char* msg);
void syntax_errorf(FileIndex index, Location loc, size_t ctx, char* fmt, ...);

void type_error(FileIndex index, Location loc, char* msg);
void type_errorf(FileIndex index, Location loc, char* fmt, ...);
void name_error(FileIndex index, Location loc, char* msg);
void name_errorf(FileIndex index, Location loc, char* fmt, ...);

void unspecified_error(FileIndex index, Location loc, char* msg);
void unspecified_errorf(FileIndex index, Location loc, char* fmt, ...);

#define UNIMPLEMENTED(msg)                                                               \
    do {                                                                                 \
        diagnostic_printf(                                                               \
            LABEL_UNIMPLEMENTED, "%s:%i:%s: %s", __FILE__, __LINE__, __func__, msg       \
        );                                                                               \
        exit(1);                                                                         \
    } while (0)

#define UNREACHABLE()                                                                    \
    do {                                                                                 \
        diagnostic_printf(LABEL_UNREACHABLE, "%s:%i:%s", __FILE__, __LINE__, __func__);  \
        exit(1);                                                                         \
    } while (0)

#endif
