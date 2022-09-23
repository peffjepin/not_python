#include "diagnostics.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODO: some of this repitition aught to be cleaned up

// automatically adds line 1 to the index at offset 0
FileIndex
create_file_index(Arena* arena, const char* filepath)
{
    FileIndex index = {.filepath = filepath, .arena = arena};
    index.line_offsets = arena_dynamic_alloc(arena, &index.bytes);
    index.line_capacity = index.bytes / sizeof(size_t);
    file_index_index_line(&index, 0);  // index line 1
    return index;
}

void
file_index_index_line(FileIndex* index, size_t line_start)
{
    if (index->line_count == index->line_capacity) {
        index->line_offsets =
            arena_dynamic_grow(index->arena, index->line_offsets, &index->bytes);
        index->line_capacity = index->bytes / sizeof(size_t);
    }
    index->line_offsets[index->line_count++] = line_start;
}

#define LINE_BUFFER_LENGTH 1024

typedef struct {
    char data[LINE_BUFFER_LENGTH];
} LineBuffer;

static void
read_source_line(
    FILE* fp, size_t line_number, size_t line_number_padding, LineBuffer* line
)
{
    // line number to string
    size_t required_length = snprintf(NULL, 0, "%zu", line_number);
    char line_number_string[required_length + 1];
    snprintf(line_number_string, required_length + 1, "%zu", line_number);
    // left pad
    size_t write = 0;
    for (; write < line_number_padding - required_length - 2; write++)
        line->data[write] = ' ';
    // write line number
    char* lns = (char*)line_number_string;
    while (*lns != '\0') line->data[write++] = *lns++;
    // write `| `
    line->data[write++] = '|';
    line->data[write++] = ' ';
    // write line
    for (; write < LINE_BUFFER_LENGTH - 1; write++) {
        char c = fgetc(fp);
        if (c == EOF) break;
        if (c == '\n') {
            line->data[write++] = '\n';
            break;
        }
        line->data[write] = c;
    }
    line->data[write] = '\0';
    // avoid printing anything for source lines that don't actually exist
    if (write == line_number_padding) line->data[0] = '\0';
}

static void
source_lines(FileIndex index, size_t line_number, size_t ctx, LineBuffer* line_buffers)
{
    FILE* fp = fopen(index.filepath, "r");
    if (!fp)
        errorf("failed to open (%s) for reading: %s", index.filepath, strerror(errno));
    if (line_number == 0)
        error("can't get source line number 0, expecting lines to start at 1");
    if (line_number - 1 >= index.line_count)
        errorf(
            "line number (%zu) out of range for file (%s)", line_number, index.filepath
        );

    int start_line = line_number - ctx;
    size_t first_line_buffer = 0;
    if (start_line <= 0) {
        first_line_buffer = 1 - start_line;
        start_line = 1;
    }
    fseek(fp, index.line_offsets[start_line - 1], 0);

    size_t maxline = line_number + ctx;
    size_t line_number_required_length = snprintf(NULL, 0, "%zu", maxline);

    for (int i = first_line_buffer; i < 1 + 2 * (int)ctx; i++) {
        read_source_line(
            fp,
            start_line + i - first_line_buffer,
            line_number_required_length + 4,
            line_buffers + i
        );
    }

    fclose(fp);
}

#define TERMINAL_BLACK "\033[0;30m"
#define TERMINAL_RED "\033[0;31m"
#define TERMINAL_GREEN "\033[0;32m"
#define TERMINAL_YELLOW "\033[0;33m"
#define TERMINAL_BLUE "\033[0;34m"
#define TERMINAL_MAGENTA "\033[0;35m"
#define TERMINAL_CYAN "\033[0;36m"
#define TERMINAL_WHITE "\033[0;37m"
#define TERMINAL_RESET "\033[0m"

typedef enum {
    LABEL_SYNTAX_ERROR,
    LABEL_DEFAULT_ERROR,
    LABEL_TYPE_ERROR,
    LABEL_NAME_ERROR,
    LABEL_WARNING,
    LABEL_UNIMPLEMENTED_ERROR,
    LABEL_SOURCE_CODE,
    LABEL_SOURCE_CODE_HIGHLIGHTED,
    LABEL_NORMAL,
    LABEL_DEBUG,
    LABEL_COUNT,
} DiagnosticLabel;

static const char* LABELS_TEXT[LABEL_COUNT] = {
    [LABEL_DEFAULT_ERROR] = "ERROR: ",
    [LABEL_SYNTAX_ERROR] = "SyntaxError: ",
    [LABEL_TYPE_ERROR] = "TypeError: ",
    [LABEL_NAME_ERROR] = "NameError: ",
    [LABEL_UNIMPLEMENTED_ERROR] = "(exhaustive error report not yet implemented):",
    [LABEL_WARNING] = "WARNING: ",
    [LABEL_DEBUG] = "DEBUG: ",
    [LABEL_SOURCE_CODE] = "",
    [LABEL_SOURCE_CODE_HIGHLIGHTED] = "",
    [LABEL_NORMAL] = "",
};

static const char* LABELS_COLOR[LABEL_COUNT] = {
    [LABEL_DEFAULT_ERROR] = TERMINAL_RED,
    [LABEL_SYNTAX_ERROR] = TERMINAL_RED,
    [LABEL_TYPE_ERROR] = TERMINAL_RED,
    [LABEL_NAME_ERROR] = TERMINAL_RED,
    [LABEL_UNIMPLEMENTED_ERROR] = TERMINAL_RED,
    [LABEL_WARNING] = TERMINAL_YELLOW,
    [LABEL_DEBUG] = TERMINAL_MAGENTA,
    [LABEL_SOURCE_CODE] = TERMINAL_RESET,
    [LABEL_SOURCE_CODE_HIGHLIGHTED] = TERMINAL_YELLOW,
    [LABEL_NORMAL] = TERMINAL_RESET,
};

static void
eprintf(DiagnosticLabel label, char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    fprintf(stderr, "%s%s", LABELS_COLOR[label], LABELS_TEXT[label]);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, TERMINAL_RESET);

    va_end(args);
}

static void
veprintf(DiagnosticLabel label, char* fmt, va_list args, bool newline)
{
    fprintf(stderr, "%s%s", LABELS_COLOR[label], LABELS_TEXT[label]);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, TERMINAL_RESET);
    if (newline) fprintf(stderr, "\n");
}

void
error(char* msg)
{
    eprintf(LABEL_DEFAULT_ERROR, "%s\n", msg);
    exit(1);
}

void
errorf(char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    veprintf(LABEL_DEFAULT_ERROR, fmt, args, true);
    va_end(args);
    exit(1);
}

void
warn(char* msg)
{
    eprintf(LABEL_WARNING, "%s\n", msg);
}

void
warnf(char* fmt, ...)
{
    va_list vargs;
    va_start(vargs, fmt);

    veprintf(LABEL_WARNING, fmt, vargs, true);

    va_end(vargs);
}

void
debug(char* msg)
{
    eprintf(LABEL_DEBUG, "%s\n", msg);
}

void
debugf(char* fmt, ...)
{
    va_list vargs;
    va_start(vargs, fmt);

    veprintf(LABEL_DEBUG, fmt, vargs, true);

    va_end(vargs);
}

static void
print_source_code(FileIndex index, Location loc, size_t ctx)
{
    LineBuffer lines[1 + ctx * 2];
    source_lines(index, loc.line, ctx, lines);
    for (size_t i = 0; i < 1 + 2 * ctx; i++) {
        DiagnosticLabel label =
            (ctx > 0 && i == ctx) ? LABEL_SOURCE_CODE_HIGHLIGHTED : LABEL_SOURCE_CODE;
        eprintf(label, "%s", lines[i].data);
    }
}

void
syntax_error(FileIndex index, Location loc, size_t ctx, char* msg)
{
    eprintf(LABEL_SYNTAX_ERROR, "%s:%u:%u\n", loc.filepath, loc.line, loc.col);
    eprintf(LABEL_NORMAL, "%s:\n", msg);
    print_source_code(index, loc, ctx);
    exit(1);
}

void
syntax_errorf(FileIndex index, Location loc, size_t ctx, char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    eprintf(LABEL_SYNTAX_ERROR, "%s:%u:%u\n", loc.filepath, loc.line, loc.col);
    veprintf(LABEL_NORMAL, fmt, args, true);
    print_source_code(index, loc, ctx);

    va_end(args);

    exit(1);
}

void
type_error(FileIndex index, Location loc, char* msg)
{
    eprintf(LABEL_TYPE_ERROR, "%s:%u:%u\n", loc.filepath, loc.line, loc.col);
    eprintf(LABEL_NORMAL, "%s:\n", msg);
    print_source_code(index, loc, 0);
    exit(1);
}

void
type_errorf(FileIndex index, Location loc, char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    eprintf(LABEL_TYPE_ERROR, "%s:%u:%u\n", loc.filepath, loc.line, loc.col);
    veprintf(LABEL_NORMAL, fmt, args, true);
    print_source_code(index, loc, 0);

    va_end(args);

    exit(1);
}

void
name_error(FileIndex index, Location loc, char* msg)
{
    eprintf(LABEL_NAME_ERROR, "%s:%u:%u\n", loc.filepath, loc.line, loc.col);
    eprintf(LABEL_NORMAL, "%s:\n", msg);
    print_source_code(index, loc, 0);
    exit(1);
}

void
name_errorf(FileIndex index, Location loc, char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    eprintf(LABEL_NAME_ERROR, "%s:%u:%u\n", loc.filepath, loc.line, loc.col);
    veprintf(LABEL_NORMAL, fmt, args, true);
    print_source_code(index, loc, 0);

    va_end(args);

    exit(1);
}

void
unspecified_error(FileIndex index, Location loc, char* msg)
{
    eprintf(LABEL_UNIMPLEMENTED_ERROR, "%s:%u:%u\n", loc.filepath, loc.line, loc.col);
    eprintf(LABEL_NORMAL, "%s:\n", msg);
    print_source_code(index, loc, 0);
    exit(1);
}

void
unspecified_errorf(FileIndex index, Location loc, char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    eprintf(LABEL_UNIMPLEMENTED_ERROR, "%s:%u:%u\n", loc.filepath, loc.line, loc.col);
    veprintf(LABEL_NORMAL, fmt, args, true);
    print_source_code(index, loc, 0);

    va_end(args);

    exit(1);
}
