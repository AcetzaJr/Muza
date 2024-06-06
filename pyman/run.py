import subprocess


def run() -> int:
    return subprocess.run(["./build/muza"]).returncode
