# Copyright 2023 Sophos Limited. All rights reserved.

import subprocess
import sys
import shutil
from pathlib import Path


def main(argv):
    assert argv[1][0] == "@", "Expecting a parameter file"

    with open(argv[1][1:]) as params_file:
        args = params_file.read().split("\n")

    assert len(args) >= 2
    strip = args[0]
    mode = args[1]
    assert mode in ["strip", "extract_symbols", "rename"]

    for arg in args[2:]:
        if len(arg) == 0:
            continue
        src, dest = arg.split("=")
        result = None
        if mode == "strip":
            result = subprocess.run([strip, "-o", dest, src], capture_output=True, text=True)
        elif mode == "extract_symbols":
            result = subprocess.run([strip, "--only-keep-debug", "-o", dest, src], capture_output=True, text=True)

        if result and result.returncode != 0:
            print(result.stderr)

        # If we have failed to perform the operation, or we are doing a simple rename, create a file in the destination
        # We need to do this even in the extract_symbols case, because Bazel has pre-declared that as an output, so a
        # file must be created there
        if mode == "rename" or result.returncode != 0:
            Path(dest).parent.mkdir(parents=True, exist_ok=True)

            if mode in ["strip", "rename"]:
                shutil.copyfile(src, dest)
            elif mode == "extract_symbols":
                Path(dest).touch()

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
