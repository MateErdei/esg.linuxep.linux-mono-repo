#!/bin/env python

import os
import subprocess
import sys

def safe_makedir(p):
    try:
        os.makedirs(p)
    except EnvironmentError as e:
        return


EXCLUDE_LIST = (
    '.sh',
    '.json',
    '.zip',
    '.crt',
    '.xml',
    '.dat',
)


def main(argv):
    dest = argv[1]
    debug = argv[2]
    buildType = argv[3]
    stripEnv = os.environ.get("ENABLE_STRIP", None)

    ## Ensure the debug directory is created, regardless if we are doing the strip or not
    safe_makedir(debug)

    if stripEnv == "0":
        print("NOT stipping binaries due to ENABLE_STRIP=0")
        return 0
    elif stripEnv == "1":
        print("Stripping binaries due to ENABLE_STRIP=1")
    elif buildType in ("Debug", "RelWithDebInfo"):
        print("NOT stripping binaries in buildType {}".format(buildType))
        return 0
    else:
        print("Stripping binaries in buildType {}".format(buildType))

    for (base, dirs, files) in os.walk(dest):
        for f in files:
            filename, file_extension = os.path.splitext(f)
            if file_extension in EXCLUDE_LIST:
                continue
            p = os.path.join(base, f)
            debugdest = p.replace(dest, debug)+".debug"
            safe_makedir(os.path.dirname(debugdest))
            ret = subprocess.call(['strip', '--only-keep-debug', '-o', debugdest, p])
            if ret == 0:
                subprocess.call(['strip', p])

    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
