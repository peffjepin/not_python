#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "compiler.h"
#include "diagnostics.h"
#include "lexer.h"
#include "writer.h"

#if DEBUG
#include "debug.h"
#endif

#ifndef INSTALL_DIR
#define INSTALL_DIR "/usr"
#endif

#define SHORT_STR_CAPACITY 128

typedef struct {
    char data[SHORT_STR_CAPACITY];
    size_t length;
} ShortString;

#define BUILD_DIR "npc_build"

static FILE*
open_file_for_writing(char* filepath)
{
    FILE* file = fopen(filepath, "w");
    if (!file) errorf("unable to open (%s) for writing (%s)", filepath, strerror(errno));
    return file;
}

static void
make_build_directory(void)
{
    int status = mkdir(BUILD_DIR, 0777);

    if (status != 0 && errno != EEXIST)
        errorf("failed to make " BUILD_DIR " directory (%s)", strerror(errno));
}

// TODO: in the future this will need to be more robust and support multiple files
#define INTERMEDIATE_FILEPATH BUILD_DIR "/intermediate.c"

static Requirements
compile_target_to_c(char* target)
{
    Lexer lexer = lex_file(target);
    FILE* outfile = open_file_for_writing(INTERMEDIATE_FILEPATH);
    CompiledInstructions compiled = compile(&lexer);
    write_c_program(compiled, outfile);
    fclose(outfile);
    return compiled.req;
}

ShortString
default_outfile(char* target)
{
    // TODO: make this more portable/robust in the future
    ShortString rtval = {0};
    size_t offset = 0;
    size_t length = 0;
    for (size_t i = 0; i < strlen(target); i++) {
        if (target[i] == '/') offset = i + 1;
        if (target[i] == '.') {
            length = i - offset;
            break;
        }
    }
    memcpy(rtval.data, target + offset, length);
    return rtval;
}

static ShortString
shortstr_from_cstr(char* cstr)
{
    ShortString str = {.length = strlen(cstr)};
    if (str.length >= SHORT_STR_CAPACITY) {
        errorf(
            "input string (%s) overflowed ShortString capacity (%u)",
            cstr,
            SHORT_STR_CAPACITY
        );
    }
    memcpy(str.data, cstr, str.length);
    return str;
}

static void
fork_and_run_sync(char* const* argv)
{
    // TODO: portability
    pid_t child_pid = fork();
    if (child_pid < 0) errorf("unable to fork process (%s)", strerror(errno));
    if (child_pid == 0) {
        if (execvp(argv[0], argv) < 0)
            errorf("unable to exec `%s` in child (%s)", argv[0], strerror(errno));
    }
    else {
        int status;
        for (;;) {
            if (waitpid(child_pid, &status, 0) < 0) {
                errorf(
                    "failed waiting on process (pid: %i) -> %s",
                    child_pid,
                    strerror(errno)
                );
            }
            if (WIFEXITED(status)) {
                int exitcode = WEXITSTATUS(status);
                if (exitcode != 0) {
                    errorf("`%s` exited with exitcode: %i", argv[0], exitcode);
                }
                break;
            }
            if (WIFSIGNALED(status)) {
                errorf(
                    "`%s` process was terminated by a signal: %i",
                    argv[0],
                    WTERMSIG(status)
                );
            }
        }
    }
}

#define ARGV_BUILDER_CAP 256

typedef struct {
    const char* buffer[ARGV_BUILDER_CAP];
    size_t length;
} ArgvBuilder;

static void
argv_append(ArgvBuilder* argv, const char* arg)
{
    if (argv->length == ARGV_BUILDER_CAP - 1) error("argv buffer overflow");
    argv->buffer[argv->length++] = arg;
}

// NULL terminated array of args
static void
argv_extend(ArgvBuilder* argv, const char* args[])
{
    const char* arg;
    while ((arg = *args++)) argv_append(argv, arg);
}

static void
compile_to_binary(Requirements req, char* outfile)
{
    ArgvBuilder args = {0};

    argv_extend(
        &args,
        (const char*[]){
            "cc",
            "-o",
            outfile,
            (const char*)INTERMEDIATE_FILEPATH,
            (const char*)"-L" INSTALL_DIR "/lib",
            (const char*)"-I" INSTALL_DIR "/include",
            NULL,
        }
    );
#if DEBUG
    argv_append(&args, "-l:not_python_db.a");
#else
    argv_append(&args, "-l:not_python.a");
#endif

    if (req.libs[LIB_MATH]) {
        argv_append(&args, "-lm");
    }

    argv_append(&args, NULL);

    fork_and_run_sync((char* const*)args.buffer);
}

static void
run_program(char* program_name)
{
    ShortString str = {0};
    size_t offset = 0;
    if (program_name[0] != '/') {
        offset = 2;
        str.data[0] = '.';
        str.data[1] = '/';
    }
    memcpy(str.data + offset, program_name, strlen(program_name));
    char* const argv[] = {str.data, NULL};
    fork_and_run_sync(argv);
}

#if DEBUG
typedef enum {
    DEBUG_NONE,
    DEBUG_TOKENS,
    DEBUG_STATEMENTS,
    DEBUG_SCOPES,
    DEBUG_C_COMPILER
} DebugProgram;
#endif

typedef struct {
    ShortString outfile;
    ShortString target;
    bool run;
#if DEBUG
    DebugProgram debug_program;
#endif
} CommandLine;

#if DEBUG

#define DEBUG_TOKENS_FLAG "--debug-tokens"
#define DEBUG_STATEMENTS_FLAG "--debug-statements"
#define DEBUG_SCOPES_FLAG "--debug-scopes"
#define DEBUG_C_COMPILER_FLAG "--debug-c-compiler"

static void
set_cli_debug_program(CommandLine* cli, DebugProgram program)
{
    if (cli->debug_program != DEBUG_NONE) error("debug programs are mutually exclusive");
    cli->debug_program = program;
}

static bool
parse_debug_option(CommandLine* cli, char* arg)
{
    if (strcmp(arg, DEBUG_TOKENS_FLAG) == 0)
        set_cli_debug_program(cli, DEBUG_TOKENS);

    else if (strcmp(arg, DEBUG_STATEMENTS_FLAG) == 0)
        set_cli_debug_program(cli, DEBUG_STATEMENTS);

    else if (strcmp(arg, DEBUG_SCOPES_FLAG) == 0)
        set_cli_debug_program(cli, DEBUG_SCOPES);

    else if (strcmp(arg, DEBUG_C_COMPILER_FLAG) == 0)
        set_cli_debug_program(cli, DEBUG_C_COMPILER);

    else
        return false;

    return true;
}

#endif  // DEBUG

static CommandLine
parse_args(size_t argc, char** argv)
{
    (void)argc;
    CommandLine cli = {0};

    argv++;
    for (;;) {
        char* arg = *argv++;
        if (!arg) break;
        if (strcmp(arg, "--run") == 0 || strcmp(arg, "-r") == 0)
            cli.run = true;
        else if (strcmp(arg, "-o") == 0 || strcmp(arg, "--out") == 0)
            cli.outfile = shortstr_from_cstr(*argv++);
        else if (arg[0] != '-' && !cli.target.length)
            cli.target = shortstr_from_cstr(arg);
#if DEBUG
        else if (parse_debug_option(&cli, arg))
            ;
#endif
        else
            errorf("unexpected argument (%s)", arg);
    }

    // TODO: usage string
    if (!cli.target.length) errorf("no target provided");
    if (!cli.outfile.length) cli.outfile = default_outfile(cli.target.data);

    return cli;
}

int
main(int argc, char** argv)
{
    (void)argc;
    CommandLine cli = parse_args(argc, argv);

#if DEBUG
    switch (cli.debug_program) {
        case DEBUG_NONE:
            break;
        case DEBUG_TOKENS:
            debug_tokens_main(cli.target.data);
            exit(0);
        case DEBUG_STATEMENTS:
            debug_statements_main(cli.target.data);
            exit(0);
        case DEBUG_SCOPES:
            debug_scopes_main(cli.target.data);
            exit(0);
        case DEBUG_C_COMPILER:
            debug_compiler_main(cli.target.data);
            exit(0);
    }
#endif

    make_build_directory();
    Requirements req = compile_target_to_c(cli.target.data);
    compile_to_binary(req, cli.outfile.data);
    if (cli.run) run_program(cli.outfile.data);
}
