# Copyright 2023 Sophos Limited. All rights reserved.

import os
import subprocess
import sys
import shutil
from pathlib import Path

NON_BINARY_EXTENSIONS = [
    ".conf",
    ".dat",
    ".dec",
    ".ini",
    ".json",
    ".py",
    ".sh",
    ".txt",
    ".typed",
    ".crt",
]


def get_expected_rpath(base, rpath_exceptions):
    rpath = rpath_exceptions.get(base, None)
    if rpath is not None:
        return rpath
    if ".so" in base:
        return ""
    return "$ORIGIN:$ORIGIN/../lib64"


def change_rpath(path, result, patchelf, rpath_exceptions):
    if result and result.returncode != 0:
        #  probably not binary
        return
    base = os.path.basename(path)
    ext = os.path.splitext(base)[1]
    if ext in NON_BINARY_EXTENSIONS:
        #  probably not binary
        return

    rpath = get_expected_rpath(base, rpath_exceptions)
    if rpath is None:
        return

    result = subprocess.run([patchelf, "--print-rpath", path], stdout=subprocess.PIPE)
    if result.returncode != 0:
        print(f"Failed to get RPATH for {path}")
        return

    existing_rpath = result.stdout.strip().decode("UTF-8")

    if existing_rpath == rpath:
        print(f"No need to change rpath for {base} - already '{existing_rpath}'")
        return

    if rpath == "":
        if existing_rpath == "$ORIGIN":
            print(f"Ignoring $ORIGIN for rpath instead of removing it for {base}")
            return

        print(f"Attempting to remove RPATH for {base}, existing '{existing_rpath}'")
        result = subprocess.run([patchelf, "--remove-rpath", path])
    else:
        print(f"Attempting to set RPATH for {base} to {rpath} from '{existing_rpath}'")
        result = subprocess.run([patchelf, "--force-rpath", "--set-rpath", rpath, path])
    if result.returncode != 0:
        print(f"Failed to set RPATH for {path} to {rpath} from '{existing_rpath}'")


def check_disk_space():
    print("df -hl:")
    subprocess.run(["df", "-hl"])


def main(argv):
    assert argv[1][0] == "@", "Expecting a parameter file"

    with open(argv[1][1:]) as params_file:
        args = params_file.read().split("\n")

    assert len(args) >= 2
    strip = args[0]
    mode = args[1]
    assert mode in ["strip", "extract_symbols", "rename"]

    rpath_exceptions = {}
    patchelf = None

    for arg in args[2:]:
        if len(arg) == 0:
            continue

        elif arg.startswith("--rpath-exception:"):
            exception = arg[len("--rpath-exception:"):]
            f, rpath = exception.split("=", 1)
            rpath_exceptions[f] = rpath
            print(f"Using rpath exception: {f}={rpath}")
            continue
        elif arg.startswith("--patchelf="):
            patchelf = arg[len("--patchelf="):]
            print(f"Using patchelf={patchelf}")
            if not os.path.isfile(patchelf):
                print(f"ERROR: Can't find patchelf: {patchelf} from {os.getcwd()}")
            continue

        src, dest = arg.split("=", 1)
        result = None
        if mode == "strip":
            result = subprocess.run([strip, "-o", dest, src], capture_output=True, text=True)
        elif mode == "extract_symbols":
            result = subprocess.run([strip, "--only-keep-debug", "-o", dest, src], capture_output=True, text=True)

        # If we have failed to perform the operation, or we are doing a simple rename, create a file in the destination
        # We need to do this even in the extract_symbols case, because Bazel has pre-declared that as an output, so a
        # file must be created there
        if mode == "rename" or result.returncode != 0:
            Path(dest).parent.mkdir(parents=True, exist_ok=True)

            if mode in ["strip", "rename"]:
                shutil.copyfile(src, dest)
            elif mode == "extract_symbols":
                Path(dest).touch()

        if patchelf is not None and mode in ["strip", "rename"]:
            change_rpath(dest, result, patchelf, rpath_exceptions)

    check_disk_space()

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
