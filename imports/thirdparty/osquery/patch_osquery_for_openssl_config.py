# Copyright 2023-2024 Sophos Limited. All rights reserved.

import sys


def main(argv):
    with open(argv[1], "rb") as f:
        osquery_binary = f.read()

    def fill_with_nulls(search):
        nonlocal osquery_binary
        index = osquery_binary.find(search)
        if index == -1:
            raise Exception(f"osquery binary does not contain {search}")
        osquery_binary = osquery_binary[:index] + (b"\0" * len(search)) + osquery_binary[index + len(search) :]

    fill_with_nulls(
        b"\0/__w/osquery/osquery/workspace/usr/src/debug/osquery/build/installed_formulas/openssl/etc/openssl\0"
    )
    fill_with_nulls(b"\0openssl.cnf\0")

    with open(argv[2], "wb") as f:
        f.write(osquery_binary)

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
