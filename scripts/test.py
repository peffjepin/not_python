#!/usr/bin/env python3

import argparse
import pathlib
import hashlib
import subprocess
import dataclasses
import shlex

ROOT = pathlib.Path(__file__).parent.parent
CHECKSUMS_FILE = ROOT / "test/checksums.dict"
VERIFIED_OUTPUT = ROOT / "test/verified_output"
LINE_LENGTH = 80


@dataclasses.dataclass
class TestKey:
    command_line: str
    sample_file: str

    def __init__(self, command_line, sample_file):
        self.command_line = command_line
        self.sample_file = str(
            self.resolve_sample_file_relative_to_root(sample_file)
        )

    def __str__(self):
        return self.command_line.replace("--file", str(self.sample_file))

    def strip_executable_path_down_to_name(self, executable):
        return pathlib.Path(executable).name

    def resolve_sample_file_relative_to_root(self, sample_file):
        return pathlib.Path(sample_file).absolute().resolve().relative_to(ROOT)


@dataclasses.dataclass
class Result:
    total_cases: int = 0
    passed: int = 0
    failed: int = 0
    updated: int = 0


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
    print("=" * LINE_LENGTH)
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


def handle_test_case_fails(test_key, checksum, output, result, interactive):
    print_failing_test_case(test_key, output)
    if interactive:
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


def main():
    parser = argparse.ArgumentParser(
        description="runs a command across every file in a given directory"
        "and compares the output with previously verified output"
    )
    parser.add_argument(
        "-c",
        "--command",
        help="the command to be run where `--file` will be replaced"
        " with a filename from the given directory",
    )
    parser.add_argument(
        "-d",
        "--directory",
        help="the directory containing the files to be tested",
    )
    parser.add_argument(
        "-i",
        "--interactive",
        action="store_true",
        help="use this flag when you want to update verified output",
    )

    args = parser.parse_args()

    samples_directory_to_test = pathlib.Path(args.directory)
    result = Result()

    for fp in samples_directory_to_test.iterdir():
        result.total_cases += 1
        md5 = hashlib.md5()
        test_key = TestKey(args.command, fp)
        p = subprocess.run(
            shlex.split(args.command.replace("--file", str(fp))),
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )
        md5.update(p.stdout)
        md5.update(str(p.returncode).encode("utf-8"))
        checksum = md5.hexdigest()
        exitcode_tag = f"\nexitcode={p.returncode}"
        output = p.stdout.decode("utf-8") + exitcode_tag

        if not VerifiedChecksums.present(test_key):
            handle_no_previous_verification_present(
                test_key, checksum, output, result, args.interactive
            )
        elif not VerifiedChecksums.verify(test_key, checksum):
            handle_test_case_fails(
                test_key, checksum, output, result, args.interactive
            )
        else:
            result.passed += 1

    print("=" * LINE_LENGTH)
    print(
        f"Ran {result.total_cases} tests: {result.passed} passed, {result.failed} failed"
    )
    if result.updated:
        print(
            f"{result.updated} test cases had their verified results updated"
        )
    print("")
    VerifiedChecksums.dump()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
