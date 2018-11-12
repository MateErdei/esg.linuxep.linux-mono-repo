#!/bin/env python

import os
import subprocess
import sys

def safe_makedir(p):
    try:
        os.makedirs(p)
    except EnvironmentError as e:
        return

def main(argv):
    dest = argv[1]
    debug = argv[2]
    buildType = argv[3]

    if os.environ.get("ENABLE_STRIP", "1") == "0" or buildType in ("Debug", "RelWithDebInfo"):
        print("NOT stipping binaries in buildType %s"%buildType)
        return 0

    print("Stripping binaries in buildType %s"%buildType)

    for (base, dirs, files) in os.walk(dest):
        for f in files:
            p = os.path.join(base, f)
            debugdest = p.replace(dest, debug)
            safe_makedir(os.path.dirname(debugdest))
            subprocess.call(['strip', '--only-keep-debug', '-o', debugdest, p])
            subprocess.call(['strip', p])

    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
