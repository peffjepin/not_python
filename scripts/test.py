#!/usr/bin/env python3

import typing
import argparse
import pathlib
import hashlib
import subprocess
import dataclasses

ROOT = pathlib.Path(__file__).parent.parent
CHECKSUMS_FILE = ROOT / "test/checksums.dict"
VERIFIED_OUTPUT = ROOT / "test/verified_output"
LINE_LENGTH = 80


@dataclasses.dataclass
class TestKey:
    command_line: str
    sample_file: pathlib.Path

    def __init__(self, command, sample_file):
        self.command = command
        self.sample_file = self.resolve_sample_file_relative_to_root(
            sample_file)

    def __str__(self):
        return f"{self.command} {str(self.sample_file)}"

    def strip_executable_path_down_to_name(self, executable):
        return pathlib.Path(executable).name

    def resolve_sample_file_relative_to_root(self, sample_file):
        return pathlib.Path(sample_file).absolute().resolve().relative_to(ROOT)


@dataclasses.dataclass
class Result:
    command: str
    total_cases: int = 0
    passed: int = 0
    failed: int = 0
    updated: int = 0

    def __str__(self):
        lines = [
            "=" * LINE_LENGTH,
            f"[{self.command}]: Ran {self.total_cases} tests: {self.passed} passed, {self.failed} failed"
        ]
        if self.updated:
            lines.append(
                f"{self.updated} test cases had their verified results updated"
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
    _dict = eval(CHECKSUMS_FILE.read_text())
    changed = False

    @classmethod
    def verify(cls, test_key, checksum):
        return cls._dict[str(test_key)] == checksum

    @classmethod
    def present(cls, test_key):
        return str(test_key) in cls._dict

    @classmethod
    def update(cls, test_key, checksum, verified_output):
        cls.changed = True
        cls._dict[str(test_key)] = checksum
        cls.verified_output_filepath(test_key).write_text(verified_output)

    @classmethod
    def verified_output_filepath(cls, test_key):
        return VERIFIED_OUTPUT / str(test_key).replace("/", "_").replace(
            " ", "__"
        )

    @classmethod
    def dump(cls):
        if cls.changed:
            CHECKSUMS_FILE.write_text(repr(cls._dict))


def print_labeled_separator(label):
    label = str(label)
    if len(label) > LINE_LENGTH - 2:
        print(label)

    lpad = (LINE_LENGTH - len(label)) // 2
    rpad = LINE_LENGTH - lpad - len(label)
    print(("-" * lpad) + label + ("-" * rpad))


def verify_new_entry_with_user(test_key, checksum, output):
    print("=" * LINE_LENGTH)
    print("NO VERIFIED OUTPUT PRESENT FOR THIS SAMPLE:")
    print_labeled_separator(test_key)
    print((ROOT / test_key.sample_file).read_text())
    print_labeled_separator("OUTPUT")
    print(output)
    answer = input("enter 'y' to confirm this output is valid: ")
    if answer.strip().lower() != "y":
        return False
    VerifiedChecksums.update(test_key, checksum, output)
    return True


def print_failing_test_case(test_key, output):
    print("#" * LINE_LENGTH * 2)
    print("FAILED TEST CASE:")
    print_labeled_separator(test_key)
    print((ROOT / test_key.sample_file).read_text())
    print_labeled_separator("OUTPUT")
    print(output)
    print_labeled_separator("EXPECTED")
    print(VerifiedChecksums.verified_output_filepath(test_key).read_text())


def handle_no_previous_verification_present(
    test_key, checksum, output, result, interactive
):
    if interactive:
        if verify_new_entry_with_user(test_key, checksum, output):
            result.passed += 1
            result.updated += 1
        else:
            result.failed += 1
    else:
        print(
            f"NO VERIFIED OUTPUT: {test_key} -- run in interactive mode to update"
        )
        result.failed += 1


def handle_test_case_fails(test_key, checksum, output, result, opts):
    print_failing_test_case(test_key, output)
    if opts.interactive:
        if opts.force_update:
            answer = "update"
        else:
            answer = input(
                "FAILING TEST CASE -- enter 'update' to verify this output as correct: "
            )
        if answer.strip().lower() == "update":
            VerifiedChecksums.update(test_key, checksum, output)
            result.updated += 1
            result.passed += 1
        else:
            result.failed += 1
    else:
        result.failed += 1

    if opts.exit_early:
        exit(1)


ROOT = pathlib.Path(__file__).parent.parent

COMMANDS_AND_TESTDIRS = {
    "debug-tokens": ROOT / "test/samples/tokenization",
    "debug-scopes": ROOT / "test/samples/scoping",
    "debug-statements": ROOT / "test/samples/statements",
    "debug-c-compiler": ROOT / "test/samples/compiler",
    "programs": ROOT / "test/samples/programs",
}


class Options(typing.NamedTuple):
    verbose: bool
    interactive: bool
    force_update: bool
    exit_early: bool


class TestGroup:
    def __init__(self, npc, command, directory):
        self.npc = npc
        self.command = command
        self.directory = (
            pathlib.Path(directory)
            if directory
            else COMMANDS_AND_TESTDIRS[command]
        )

    def execute(self, opts):
        result = Result(self.command)

        for fp in self.directory.iterdir():
            result.total_cases += 1
            md5 = hashlib.md5()
            test_key = TestKey(self.command, fp)
            command = self.command if self.command != "programs" else "run"
            p = subprocess.run(
                [self.npc, "-o", "testmain", f"--{command}", str(fp)],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
            )
            try:
                output = p.stdout.decode("utf-8").replace(str(test_key.sample_file.absolute()),
                                                          str(test_key.sample_file))
                md5.update(output.encode("utf-8"))
                md5.update(str(p.returncode).encode("utf-8"))
                checksum = md5.hexdigest()
                exitcode_tag = f"\nexitcode={p.returncode}"
                output += exitcode_tag
            except Exception:
                print(f"failed to decode program output ({str(fp)}):")
                print(p.stdout)
                exit(1)

            if not VerifiedChecksums.present(test_key):
                handle_no_previous_verification_present(
                    test_key, checksum, output, result, opts.interactive
                )
            elif not VerifiedChecksums.verify(test_key, checksum):
                handle_test_case_fails(
                    test_key, checksum, output, result, opts
                )
            else:
                if opts.verbose:
                    print(f"[PASS]: {test_key}")
                result.passed += 1

        return result


def init_test_groups(args):
    tests = []
    if args.debug_tokens:
        tests.append(TestGroup(args.npc, "debug-tokens", args.directory))
    if args.debug_statements:
        tests.append(TestGroup(args.npc, "debug-statements", args.directory))
    if args.debug_scopes:
        tests.append(TestGroup(args.npc, "debug-scopes", args.directory))
    if args.programs:
        tests.append(TestGroup(args.npc, "programs", args.directory))
    if args.debug_c_compiler:
        tests.append(TestGroup(args.npc, "debug-c-compiler", args.directory))
    if not tests:
        tests.append(TestGroup(args.npc, "debug-tokens", args.directory))
        tests.append(TestGroup(args.npc, "debug-statements", args.directory))
        tests.append(TestGroup(args.npc, "debug-scopes", args.directory))
        tests.append(TestGroup(args.npc, "programs", args.directory))
        tests.append(TestGroup(args.npc, "debug-c-compiler", args.directory))
    return tests


def main():
    parser = argparse.ArgumentParser(
        description="runs a command across every file in a given directory"
        "and compares the output with previously verified output"
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
        "--debug_c_compiler",
        action="store_true",
        help="runs the debug_c_compiler program",
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

    args = parser.parse_args()
    tests = init_test_groups(args)
    opts = Options(
        verbose=args.verbose,
        exit_early=args.exit_early,
        force_update=args.force_update,
        interactive=args.interactive,
    )
    accumulative_result = Result("test.py")
    for tg in tests:
        result = tg.execute(opts)
        accumulative_result += result
        print(result)
    print(accumulative_result)
    VerifiedChecksums.dump()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
