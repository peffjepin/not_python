#!/usr/bin/env python3

import tempfile
import types
import sys
import os
import shutil
import itertools
import typing
import time
import shlex
import argparse
import pathlib
import hashlib
import subprocess
import dataclasses

PROJECT_ROOT = pathlib.Path(__file__).parent.parent
TMP = PROJECT_ROOT / "test_tmp"
CHECKSUMS_FILE = PROJECT_ROOT / "test/checksums.dict"
VERIFIED_OUTPUT = PROJECT_ROOT / "test/verified_output"
VERIFIED_OUTPUT.mkdir(parents=True, exist_ok=True)
LINE_LENGTH = 80
POLL = 1 / 1000
counter = itertools.count(1)

TERMINAL_RED = "\033[0;31m"
TERMINAL_GREEN = "\033[0;32m"
TERMINAL_YELLOW = "\033[0;33m"
TERMINAL_MAGENTA = "\033[0;35m"
TERMINAL_CYAN = "\033[0;36m"
TERMINAL_RESET = "\033[0m"


def resolve_path_relative_to_root(sample_file):
    return pathlib.Path(sample_file).absolute().resolve().relative_to(PROJECT_ROOT)


@dataclasses.dataclass
class Result:
    test_label: str
    total_cases: int = 0
    passed: int = 0
    failed: int = 0
    updated: int = 0
    is_final_result: bool = False

    def __str__(self):
        label_color = TERMINAL_RED if self.failed else TERMINAL_YELLOW if self.updated else TERMINAL_GREEN
        lines = [
            f"[{label_color}{self.test_label}{TERMINAL_RESET}]: Ran {self.total_cases} tests: "
            f"{TERMINAL_GREEN if self.is_final_result else ''}{self.passed} passed{TERMINAL_RESET if self.is_final_result else ''}, "
            f"{TERMINAL_RED if self.is_final_result else ''}{self.failed} failed{TERMINAL_RESET if self.is_final_result else ''}"
        ]
        if self.updated:
            lines.append(
                f"    {self.updated} test cases had their verified results updated"
            )
        lines.append("")
        return "\n".join(lines)

    def __iadd__(self, other):
        self.total_cases += other.total_cases
        self.passed += other.passed
        self.failed += other.failed
        self.updated += other.updated
        return self


class VerifiedChecksums:
    if not CHECKSUMS_FILE.exists():
        CHECKSUMS_FILE.write_text("{}")

    _dict = eval(CHECKSUMS_FILE.read_text())
    changed = False

    @staticmethod
    def _clean_key(test_key: str):
        return (
            test_key.replace(str(PROJECT_ROOT.absolute()) + os.sep, "")
            .replace("/", "_")
            .replace(" ", "__")
        )

    @classmethod
    def verify(cls, test_key, checksum):
        return cls._dict[cls._clean_key(test_key)] == checksum

    @classmethod
    def present(cls, test_key):
        return cls._clean_key(test_key) in cls._dict

    @classmethod
    def update(cls, test_key, checksum, verified_output):
        cls.changed = True
        key = cls._clean_key(test_key)
        cls._dict[key] = checksum
        cls.verified_output_filepath(key).write_text(verified_output)

    @classmethod
    def verified_output_filepath(cls, test_key):
        return VERIFIED_OUTPUT / cls._clean_key(test_key)

    @classmethod
    def dump(cls):
        if cls.changed:
            CHECKSUMS_FILE.write_text(repr(cls._dict))


def print_labeled_separator(label):
    if len(label) > LINE_LENGTH - 2:
        print(label)
    print(label.center(LINE_LENGTH, "="))


def clear_shell():
    if os.name == "nt":
        os.system("cls")
    else:
        os.system("clear")


def verify_new_entry_with_user(test_key, checksum, output):
    clear_shell()
    print("NO VERIFIED OUTPUT PRESENT FOR THIS SAMPLE:")
    print_labeled_separator(test_key)

    file = PROJECT_ROOT / test_key.split()[-1]
    if file.exists():
        for i, line in enumerate(file.read_text().splitlines()):
            print(f"{i+1}: ", line)

    print_labeled_separator("OUTPUT")
    print(output)
    answer = input("enter 'y' to confirm this output is valid: ")
    if answer.strip().lower() != "y":
        return False
    VerifiedChecksums.update(test_key, checksum, output)
    return True


def print_failing_test_case(test_key, output):
    print("#" * int(LINE_LENGTH * 1.3))

    print("FAILED TEST CASE:")
    print_labeled_separator(test_key)
    file = PROJECT_ROOT / test_key.split()[-1]
    if file.exists():
        print(file.read_text())

    print_labeled_separator("OUTPUT")
    print(output)

    print_labeled_separator("EXPECTED")
    print(VerifiedChecksums.verified_output_filepath(test_key).read_text())


class Options(typing.NamedTuple):
    verbose: bool
    interactive: bool
    force_update: bool
    exit_early: bool
    sync: bool


class TestCase(typing.NamedTuple):
    test_key: str
    sp: subprocess.Popen


class TestRunner:
    def __init__(self, label: str, opts: Options):
        self.opts = opts
        self.test_cases = []
        self.completed = 0
        self.result = Result(label)

    def run(self, cwd: str | pathlib.Path, test_key: str, command: str):
        if self.opts.sync:
            self._run(cwd, test_key, command)
        else:
            self._run_async(cwd, test_key, command)

    def _run_async(self, cwd: str | pathlib.Path, test_key: str, command: str):
        self.test_cases.append(
            TestCase(
                test_key=test_key,
                sp=subprocess.Popen(
                    shlex.split(command),
                    cwd=cwd,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                ),
            )
        )

    def _run(self, cwd: str | pathlib.Path, test_key: str, command: str):
        tc = TestCase(
            test_key=test_key,
            sp=subprocess.Popen(
                shlex.split(command),
                cwd=cwd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
            ),
        )
        tc.sp.wait()
        self._collect_result(tc)

    def poll(self):
        test_cases = []
        for tc in self.test_cases:
            if tc.sp.poll() is not None:
                self._collect_result(tc)
            else:
                test_cases.append(tc)
        self.test_cases = test_cases

    @property
    def finished(self):
        return self.completed == len(self.test_cases)

    def _collect_result(self, test_case: TestCase):
        self.result.total_cases += 1
        try:
            md5 = hashlib.md5()
            raw = test_case.sp.stdout.read()
            output = raw.decode("utf-8").replace(
                str(PROJECT_ROOT.absolute()) + os.sep, ""
            )
            md5.update(output.encode("utf-8"))
            md5.update(str(test_case.sp.returncode).encode("utf-8"))
            checksum = md5.hexdigest()
            exitcode_tag = f"\nexitcode={test_case.sp.returncode}"
            output += exitcode_tag
        except Exception as exc:
            print(f"failed to decode program output ({test_case.test_key}):")
            print(exc)
            print(raw)
            self.result.failed += 1
            return

        if not VerifiedChecksums.present(test_case.test_key):
            self._handle_no_previous_verification_present(
                test_case.test_key, checksum, output
            )
        elif not VerifiedChecksums.verify(test_case.test_key, checksum):
            self._handle_test_case_fails(test_case.test_key, checksum, output)
        else:
            if self.opts.verbose:
                print(f"[PASS]: {test_case.test_key}")
            self.result.passed += 1

    def _handle_no_previous_verification_present(
        self, test_key: str, checksum: str, output: str
    ):
        if self.opts.interactive:
            if verify_new_entry_with_user(test_key, checksum, output):
                self.result.passed += 1
                self.result.updated += 1
            else:
                self.result.failed += 1
        else:
            print(
                f"NO VERIFIED OUTPUT: {test_key} -- run in interactive mode to update"
            )
            self.result.failed += 1

    def _handle_test_case_fails(self, test_key, checksum, output):
        if self.opts.interactive:
            clear_shell()
        print_failing_test_case(test_key, output)
        if self.opts.interactive:
            if self.opts.force_update:
                answer = "u"
            else:
                answer = input(
                    "FAILING TEST CASE -- enter 'u' to verify this output as correct: "
                )
            if answer.strip().lower() == "u":
                VerifiedChecksums.update(test_key, checksum, output)
                self.result.updated += 1
                self.result.passed += 1
            else:
                self.result.failed += 1
        else:
            self.result.failed += 1

        if self.opts.exit_early:
            raise SystemExit(1)


class CompilerIterdirGroup:
    def __init__(
        self,
        directory: pathlib.Path,
        cli_flags: str,
        args: types.SimpleNamespace,
        opts: Options,
    ):
        assert directory.is_dir(), str(directory)
        self.runner = TestRunner(directory.relative_to(PROJECT_ROOT), opts)
        self.npc = str(pathlib.Path(args.npc).absolute())
        self.cli_flags = cli_flags
        self.directory = directory
        """
        self.directory = (
            pathlib.Path(args.directory)
            if args.directory
            else self.FLAGS_TO_DEFAULT_DIR[cli_flags]
        )
        """

    def begin(self):
        for fp in self.directory.iterdir():
            cwd = TMP / str(next(counter))
            cwd.mkdir(parents=True)
            self.runner.run(
                cwd=cwd,
                test_key=f"{self.cli_flags.strip('-')} {fp.relative_to(PROJECT_ROOT)}",
                command=f"{self.npc} -o testmain {self.cli_flags} {str(fp.absolute())}",
            )

    @property
    def result(self):
        if not self.runner.finished:
            self.runner.poll()
        if self.runner.finished:
            return self.runner.result
        return None


def init_test_groups(args, opts):
    tests = []
    outer_dirs = (PROJECT_ROOT / "test/features", PROJECT_ROOT / "test/errors")
    for d in outer_dirs:
        for test_dir in d.iterdir():
            tests.append(CompilerIterdirGroup(
                test_dir, "--run", args, opts))
    """
    if args.programs:
        tests.append(CompilerIterdirGroup(
            PROJECT_ROOT/"test/samples/programs", "--run", args, opts))
    if args.debug_tokens:
        tests.append(
            CompilerIterdirGroup(
                PROJECT_ROOT/"test/samples/tokenization", "--debug-tokens", args, opts)
        )
    if args.debug_statements:
        tests.append(
            CompilerIterdirGroup(
                PROJECT_ROOT/"test/samples/statements", "--debug-statements", args, opts
            )
        )
    if args.debug_scopes:
        tests.append(
            CompilerIterdirGroup(PROJECT_ROOT/"test/samples/scoping",
                                 "--debug-scopes", args, opts)
        )
    if args.debug_compiler:
        tests.append(
            CompilerIterdirGroup(
                PROJECT_ROOT/"test/samples/compiler", "--debug-c-compiler", args, opts
            )
        )
    if not tests:
        tests.append(CompilerIterdirGroup(
            PROJECT_ROOT/"test/samples/programs", "--run", args, opts))
        tests.append(
            CompilerIterdirGroup(
                PROJECT_ROOT/"test/samples/tokenization", "--debug-tokens", args, opts)
        )
        tests.append(
            CompilerIterdirGroup(
                PROJECT_ROOT/"test/samples/statements", "--debug-statements", args, opts
            )
        )
        tests.append(
            CompilerIterdirGroup(PROJECT_ROOT/"test/samples/scoping",
                                 "--debug-scopes", args, opts)
        )
        tests.append(
            CompilerIterdirGroup(
                PROJECT_ROOT/"test/samples/compiler", "--debug-c-compiler", args, opts
            )
        )
    """
    return tests


def main():
    parser = argparse.ArgumentParser(
        description="runs a command across every file in a given directory"
        " and compares the output with previously verified output"
    )
    parser.add_argument(
        "--npc", default="./npc", help="the path to npc executable [./npc]"
    )
    parser.add_argument(
        "--debug_tokens",
        action="store_true",
        help="runs the debug_tokens program",
    )
    parser.add_argument(
        "--debug_scopes",
        action="store_true",
        help="runs the debug_scopes program",
    )
    parser.add_argument(
        "--programs", action="store_true", help="runs the test programs"
    )
    parser.add_argument(
        "--debug_compiler",
        action="store_true",
        help="runs the debug_compiler program",
    )
    parser.add_argument(
        "--debug_statements",
        action="store_true",
        help="runs the debug_statements program",
    )
    parser.add_argument(
        "-d",
        "--directory",
        help="the directory containing the files to be tested (optional)",
    )
    parser.add_argument(
        "-i",
        "--interactive",
        action="store_true",
        help="use this flag when you want to update verified output",
    )
    parser.add_argument(
        "-f",
        "--force-update",
        action="store_true",
        help="use this flag to force update currently failing cases",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="displays extra information",
    )
    parser.add_argument(
        "-x", "--exit_early", action="store_true", help="exit on first failure"
    )
    parser.add_argument(
        "-s", "--sync", action="store_true", help="run synchronously")
    parser.add_argument(
        "-b", "--build", action="store_true", help="rebuild the compiler")
    parser.add_argument(
        "--preserve-test-dir",
        action="store_true",
        help="don't remove the temporary test directory",
    )

    args = parser.parse_args()
    opts = Options(
        verbose=args.verbose,
        exit_early=args.exit_early,
        force_update=args.force_update,
        interactive=args.interactive,
        sync=args.sync
    )
    tests = init_test_groups(args, opts)
    accumulative_result = Result("total", is_final_result=True)

    if args.build:
        start = time.time()
        p = subprocess.run(
            shlex.split("make debug"),
            cwd=PROJECT_ROOT,
            stdout=sys.stdout,
            stderr=sys.stderr
        )
        if p.returncode != 0:
            raise SystemExit(1)
        print("building npc took:", f"{round(time.time() - start, 2)}s")

    with tempfile.TemporaryDirectory() as tmp:
        global TMP
        TMP = pathlib.Path(tmp)
        start = time.time()
        try:
            for tg in tests:
                # NOTE: this will spawn a LOT of processes unless --sync is given
                tg.begin()

            results = []
            while tests:
                next_iter = []
                for tg in tests:
                    if tg.result:
                        results.append(tg.result)
                        accumulative_result += tg.result
                    else:
                        next_iter.append(tg)
                time.sleep(POLL)
                tests = next_iter

            for r in results:
                print(r)
            print(accumulative_result)
            VerifiedChecksums.dump()
            return 0
        finally:
            print("testing took:", f"{round(time.time() - start, 2)}s")
            if args.preserve_test_dir:
                dst = PROJECT_ROOT/"testdir"
                if dst.exists():
                    shutil.rmtree(dst, ignore_errors=True)
                shutil.copytree(TMP, dst)


if __name__ == "__main__":
    raise SystemExit(main())
