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
    stripEnv = os.environ.get("ENABLE_STRIP", None)

    if stripEnv == "0":
        print("NOT stipping binaries due to ENABLE_STRIP=0")
        return 0
    elif stripEnv == "1":
        print("Stripping binaries due to ENABLE_STRIP=1")
    elif buildType in ("Debug", "RelWithDebInfo"):
        print("NOT stipping binaries in buildType %s"%buildType)
        return 0
    else:
        print("Stripping binaries in buildType %s"%buildType)

    for (base, dirs, files) in os.walk(dest):
        for f in files:
            p = os.path.join(base, f)
            debugdest = p.replace(dest, debug)
            safe_makedir(os.path.dirname(debugdest))
            ret = subprocess.call(['strip', '--only-keep-debug', '-o', debugdest, p])
            if ret == 0:
                subprocess.call(['strip', p])

    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
