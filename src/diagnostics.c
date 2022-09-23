#include "diagnostics.h"

#include <assert.h>
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
#define TERMINAL_DEFAULT TERMINAL_RESET

typedef struct {
    const char* text;
    const char* term_color;
} DiagnosticConfig;

static_assert(LABEL_COUNT == 12, "new DiagnosticLabel config required");
static const DiagnosticConfig CONFIG_TABLE[LABEL_COUNT] = {
    [LABEL_DEFAULT_ERROR] = {.text = "ERROR: ", .term_color = TERMINAL_RED},
    [LABEL_SYNTAX_ERROR] = {.text = "SyntaxError: ", .term_color = TERMINAL_RED},
    [LABEL_TYPE_ERROR] = {.text = "TypeError: ", .term_color = TERMINAL_RED},
    [LABEL_NAME_ERROR] = {.text = "NameError: ", .term_color = TERMINAL_RED},
    [LABEL_UNSPECIFIED_ERROR] =
        {.text = "(exhaustive error report not yet implemented):",
         .term_color = TERMINAL_RED},
    [LABEL_WARNING] = {.text = "WARNING: ", .term_color = TERMINAL_YELLOW},
    [LABEL_DEBUG] = {.text = "DEBUG: ", .term_color = TERMINAL_MAGENTA},
    [LABEL_SOURCE_CODE] = {.text = "", .term_color = TERMINAL_DEFAULT},
    [LABEL_SOURCE_CODE_HIGHLIGHTED] = {.text = "", .term_color = TERMINAL_YELLOW},
    [LABEL_NORMAL] = {.text = "", .term_color = TERMINAL_DEFAULT},
    [LABEL_UNIMPLEMENTED] =
        {.text = "INTERNAL COMPILER ERROR: (UNIMPLEMENTED): ",
         .term_color = TERMINAL_RED},
    [LABEL_UNREACHABLE] =
        {.text = "INTERNAL COMPILER ERROR (UNREACHABLE): ", .term_color = TERMINAL_RED},
};

void
diagnostic_printf(DiagnosticLabel label, char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    DiagnosticConfig config = CONFIG_TABLE[label];
    fprintf(stderr, "%s%s", config.term_color, config.text);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, TERMINAL_RESET);

    va_end(args);
}

void
diagnostic_vprintf(DiagnosticLabel label, char* fmt, va_list args, bool newline)
{
    DiagnosticConfig config = CONFIG_TABLE[label];
    fprintf(stderr, "%s%s", config.term_color, config.text);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, TERMINAL_RESET);
    if (newline) fprintf(stderr, "\n");
}

void
error(char* msg)
{
    diagnostic_printf(LABEL_DEFAULT_ERROR, "%s\n", msg);
    exit(1);
}

void
errorf(char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    diagnostic_vprintf(LABEL_DEFAULT_ERROR, fmt, args, true);
    va_end(args);
    exit(1);
}

void
warn(char* msg)
{
    diagnostic_printf(LABEL_WARNING, "%s\n", msg);
}

void
warnf(char* fmt, ...)
{
    va_list vargs;
    va_start(vargs, fmt);

    diagnostic_vprintf(LABEL_WARNING, fmt, vargs, true);

    va_end(vargs);
}

void
debug(char* msg)
{
    diagnostic_printf(LABEL_DEBUG, "%s\n", msg);
}

void
debugf(char* fmt, ...)
{
    va_list vargs;
    va_start(vargs, fmt);

    diagnostic_vprintf(LABEL_DEBUG, fmt, vargs, true);

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
        diagnostic_printf(label, "%s", lines[i].data);
    }
}

static void
error_with_source(
    DiagnosticLabel head_label,
    DiagnosticLabel msg_label,
    FileIndex index,
    Location loc,
    size_t ctx,
    char* msg
)
{
    diagnostic_printf(head_label, "%s:%u:%u\n", loc.filepath, loc.line, loc.col);
    diagnostic_printf(msg_label, "%s:\n", msg);
    print_source_code(index, loc, ctx);
    exit(1);
}
static void
errorf_with_source(
    DiagnosticLabel head_label,
    DiagnosticLabel msg_label,
    FileIndex index,
    Location loc,
    size_t ctx,
    char* fmt,
    va_list args
)
{
    diagnostic_printf(head_label, "%s:%u:%u\n", loc.filepath, loc.line, loc.col);
    diagnostic_vprintf(msg_label, fmt, args, true);
    print_source_code(index, loc, ctx);
    exit(1);
}

#define WITH_VA(args, last_non_va_arg)                                                   \
    va_list args;                                                                        \
    for (int i = (va_start(args, last_non_va_arg), 0); i < 1; i = (va_end(args), i + 1))

void
syntax_error(FileIndex index, Location loc, size_t ctx, char* msg)
{
    error_with_source(LABEL_SYNTAX_ERROR, LABEL_NORMAL, index, loc, ctx, msg);
}

void
syntax_errorf(FileIndex index, Location loc, size_t ctx, char* fmt, ...)
{
    WITH_VA(args, fmt)
    {
        errorf_with_source(LABEL_SYNTAX_ERROR, LABEL_NORMAL, index, loc, ctx, fmt, args);
    }
}

void
type_error(FileIndex index, Location loc, char* msg)
{
    error_with_source(LABEL_TYPE_ERROR, LABEL_NORMAL, index, loc, 0, msg);
}

void
type_errorf(FileIndex index, Location loc, char* fmt, ...)
{
    WITH_VA(args, fmt)
    {
        errorf_with_source(LABEL_TYPE_ERROR, LABEL_NORMAL, index, loc, 0, fmt, args);
    }
}

void
name_error(FileIndex index, Location loc, char* msg)
{
    error_with_source(LABEL_NAME_ERROR, LABEL_NORMAL, index, loc, 0, msg);
}

void
name_errorf(FileIndex index, Location loc, char* fmt, ...)
{
    WITH_VA(args, fmt)
    {
        errorf_with_source(LABEL_NAME_ERROR, LABEL_NORMAL, index, loc, 0, fmt, args);
    }
}

void
unspecified_error(FileIndex index, Location loc, char* msg)
{
    error_with_source(LABEL_UNSPECIFIED_ERROR, LABEL_NORMAL, index, loc, 0, msg);
}

void
unspecified_errorf(FileIndex index, Location loc, char* fmt, ...)
{
    WITH_VA(args, fmt)
    {
        errorf_with_source(
            LABEL_UNSPECIFIED_ERROR, LABEL_NORMAL, index, loc, 0, fmt, args
        );
    }
}
