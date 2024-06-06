import os
import pathlib
import subprocess


def fmtcfile(path: str) -> int:
    if (
        subprocess.run(
            ["clang-format", "--Werror", "--dry-run", path],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        ).returncode
        != 0
    ):
        if (
            subprocess.run(
                ["clang-format", "-i", path],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
            ).returncode
            == 0
        ):
            print(f"    * {path} formatted")
            return 0
        print(f"    * {path} could not be formatted")
        return 1
    return 0


def fmt() -> int:
    print("Formatting project...")
    print("Formatting py files...")
    fmtpy()
    print("Formatting cmake files...")
    fmtcmake()
    fmtallc()
    return 0


def fmtallc() -> int:
    print("Formatting c files...")
    fmtc("c/source", ".c")
    fmtc("c/include", ".h")
    return 0


def fmtc(directory: str, suffix: str) -> int:
    for dirpath, dirnames, filenames in os.walk(directory):
        for file in filenames:
            if pathlib.Path(file).suffix == suffix:
                fmtcfile(os.path.join(dirpath, file))
    return 0


def fmtpyfile(path: str) -> int:
    if (
        subprocess.run(
            ["black", "--check", path],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        ).returncode
        != 0
    ):
        if (
            subprocess.run(
                ["black", path],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
            ).returncode
            == 0
        ):
            print(f"    * {path} formatted")
            return 0
        print(f"    * {path} could not be formatted")
        return 1
    return 0


def fmtpy() -> int:
    subprocess.run(["black", "."])
    subprocess.run(["isort", "."])


def fmtcmake() -> int:
    fmtcmakefile("CMakeLists.txt")
    return 0


def fmtcmakefile(path: str) -> int:
    if (
        subprocess.run(
            ["cmake-format", "--check", path],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        ).returncode
        != 0
    ):
        if (
            subprocess.run(
                ["cmake-format", "-i", path],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
            ).returncode
            == 0
        ):
            print(f"    * {path} formatted")
            return 0
        print(f"    * {path} could not be formatted")
        return 1
    return 0
