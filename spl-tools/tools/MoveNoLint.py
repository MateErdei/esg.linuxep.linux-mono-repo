#!/bin/env python
from __future__ import absolute_import, print_function, division, unicode_literals

import re
import os
import sys


TEST_RE = re.compile(r" *TEST.*")
END_TEST_NOLINE_RE = re.compile(r"(.*) *// *NOLINT")


def processFile(filepath):
    lines = open(filepath).readlines()
    outlines = []
    for line in lines:
        nolint = False
        mo = TEST_RE.match(line)
        if mo:
            line = line.rstrip()
            if line.endswith(b"NOLINT"):
                outlines.append(line+b"\n")
                continue
            outlines.append(line+b" // NOLINT\n")
            continue

        nolint = (
            b"EXPECT_THROW" in line or
            b"ASSERT_THROW" in line or
            b"EXPECT_NO_THROW" in line or
            b"ASSERT_NO_THROW" in line or
            b"INSTANTIATE_TEST_CASE_P" in line or
            b"EXPECT_EXIT" in line
        )

        if nolint:
            if "NOLINT" not in line:
                line = line.rstrip()
                outlines.append(line+b" // NOLINT\n")
                continue
        else:
            # Remove NOLINT
            mo = END_TEST_NOLINE_RE.match(line)
            if mo:
                outlines.append(mo.group(1)+b"\n")
                continue

        if line == b'#include "gtest/gtest.h"\n':
            line = b'#include <gtest/gtest.h>\n'

        if line == b'#include "gmock/gmock.h"\n':
            line = b'#include <gmock/gmock.h>\n'

        outlines.append(line)

    if lines != outlines:
        print(filepath)
        open(filepath, "w").writelines(outlines)


def main(argv):
    for (base, dirs, files) in os.walk(argv[1]):
        for f in files:
            if not f.endswith(".cpp"):
                continue
            processFile(os.path.join(base,f))

    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
