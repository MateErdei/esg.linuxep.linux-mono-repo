#!/usr/bin/env python

from __future__ import print_function,division,unicode_literals

import os
import sys

def main(argv):
    src = argv[1]
    dest = argv[2]
    corruption_type = argv[3]

    lines = open(src).readlines()
    lineNum = -1

    if corruption_type in ("1","2"):
        # Manifest.dat with bad certificate produced by modifying first byte of certificate M -> N.
        for (i,line) in enumerate(lines):
            if "-----BEGIN CERTIFICATE-----" in line:
                lineNum = i+1
                if corruption_type == "1":
                    break
        lines[lineNum] = "N" + lines[lineNum][1:]
    elif corruption_type == "S":
        # Manifest.dat with bad signature produced by modifying first byte of second line k -> l.
        for (i,line) in enumerate(lines):
            if "-----BEGIN SIGNATURE-----" in line:
                lineNum = i+2
                break
        lines[lineNum] = "l" + lines[lineNum][1:]

    open(dest,"w").writelines(lines)
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))

