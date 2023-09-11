# Copyright 2020-2023 Sophos Limited. All rights reserved.
import argparse
import subprocess
import os


def get_base_version(args):
    return args.base_version or args.base_version_file.readline().strip()


def run(args):
    if os.path.isfile(args.bazel_stamp_vars):
        with open(args.bazel_stamp_vars) as f:
            lines = f.read().split("\n")
            for line in lines:
                if len(line) == 0:
                    continue
                var = line.split(" ")[0]
                value = " ".join(line.split(" ")[1:])

                if var == "CI_SIGNING_URL":
                    os.environ["CI_SIGNING_URL"] = value
                if var == "BUILD_JWT_PATH":
                    os.environ["BUILD_JWT_PATH"] = value
                if var == "SOURCE_CODE_BRANCH":
                    os.environ["SOURCE_CODE_BRANCH"] = value

    res = subprocess.run(
        ["versioning_client", "-c", args.component_name, "-v", get_base_version(args)],
        check=True,
        capture_output=True,
        universal_newlines=True,
        encoding="UTF8",
    )
    version = res.stdout.strip()
    for line in args.input:
        replaced = line.replace(args.token, version)
        args.output.writelines([replaced])


def main():
    parser = argparse.ArgumentParser("Expand the file with the version returned by the versioning client")
    parser.add_argument("--component_name", required=True)
    base_version_group = parser.add_mutually_exclusive_group(required=True)
    base_version_group.add_argument("--base_version")
    base_version_group.add_argument("--base_version_file", type=open)
    parser.add_argument("--input", type=open, required=True)
    parser.add_argument("--output", type=argparse.FileType("w"), required=True)
    parser.add_argument("--token", required=True)
    parser.add_argument("--bazel_stamp_vars")
    args = parser.parse_args()
    run(args)


if __name__ == "__main__":
    main()
