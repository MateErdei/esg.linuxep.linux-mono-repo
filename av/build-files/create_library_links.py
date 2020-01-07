#!/usr/bin/env python3

import os
import re
import sys
LIBRARY_RE = re.compile(r"(lib.*.so.*)\.\d+")


def process_file(f):
    d = os.path.dirname(f)
    b = os.path.basename(f)

    while True:
        mo = LIBRARY_RE.match(b)
        if not mo:
            return

        dest = os.path.join(d, mo.group(1))
        if not os.path.isfile(dest):
            os.symlink(b, dest)
            print("Linked %s to %s" % (dest, b))

        b = mo.group(1)


def process(d):
    for (base, dirs, files) in os.walk(d):
        for f in files:
            process_file(os.path.join(base, f))


def main(argv):
    for d in argv:
        process(d)
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
