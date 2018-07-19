#!/usr/bin/env python

from __future__ import print_function,division,unicode_literals

import os
import re
import sys

def getHeaderGuard(p):
    parts = []
    h = p
    while True:
        (h,t) = os.path.split(h)
        if t in ("modules","tests","products"):
            break
        parts.append(t.upper().replace(".","_"))
    parts.reverse()
    return "_".join(parts)

## Replace first ifndef
IFNDEF_RE = re.compile(r"#ifndef ([A-Z_]+)")

def fixup(p):
    f = open(p)
    lines = f.readlines()
    f.close()
    output = []
    guard = getHeaderGuard(p)
    oldguard = None
    print("Fixing up %s with %s"%(p,guard))
    for line in lines:
        if oldguard is not None:
            output.append(line.replace(oldguard,guard))
            continue

        mo = IFNDEF_RE.match(line)
        if not mo:
            output.append(line)
            continue

        oldguard = mo.group(1)
        output.append(line.replace(oldguard,guard))

    f = open(p,"w")
    f.writelines(output)
    f.close()


def main(argv):
    for p in argv[1:]:
        fixup(p)

    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))

