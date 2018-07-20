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
        if t in (b"modules",b"tests",b"products"):
            break
        parts.append(t.upper().replace(b".",b"_"))
    parts.reverse()
    return b"_".join(parts)

## Replace first ifndef
IFNDEF_RE = re.compile(br"#ifndef ([A-Z_]+)")


def fixup(p):
    # type: (str) -> None
    if os.path.isdir(p):
        for f in os.listdir(p):
            if f in (".", ".."):
                continue
            fixup(os.path.join(p, f))

    elif not os.path.isfile(p):
        return

    if not p.endswith(".h"):
        return

    f = open(p)
    lines = f.readlines()
    f.close()
    output = []
    guard = getHeaderGuard(p)
    oldguard = None
    changed = False
    for line in lines:
        if oldguard is not None:
            output.append(line.replace(oldguard,guard))
            continue

        mo = IFNDEF_RE.match(line)
        if not mo:
            output.append(line)
            continue

        oldguard = mo.group(1)
        if oldguard != guard:
            changed = True
        output.append(line.replace(oldguard,guard))

    if changed:
        print("Fixing up %s with %s"%(p,guard))
        f = open(p,"w")
        f.writelines(output)
        f.close()


def main(argv):
    for p in argv[1:]:
        fixup(p)

    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))

