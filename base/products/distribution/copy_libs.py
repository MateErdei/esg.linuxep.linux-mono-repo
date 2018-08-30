#!/bin/env python

import os
import sys
import shutil
import filecmp
import re

DIGIT_EXT = re.compile(r"^(.*?)\.\d+$")


def safe_symlink(source, target):
    try:
        os.symlink(source,target)
    except EnvironmentError as e:
        if e.errno == 17:
            return
        print(e)


def main(argv):
    dest = argv[1]
    targets = argv[2:]

    touchedFiles = []

    for t in targets:
        assert os.path.isfile(t)
        base = os.path.basename(t)
        target_dest = os.path.join(dest, base)
        touchedFiles.append(base)
        ## Copy if changed
        if not (os.path.isfile(target_dest) and filecmp.cmp(t, target_dest)):
            shutil.copy2(t, target_dest)
        ## Create symlinks
        target = base
        while True:
            mo = DIGIT_EXT.match(target)
            if not mo:
                break
            # print(target,mo.group(1))
            safe_symlink(target, os.path.join(dest,mo.group(1)))
            target = mo.group(1)

    ## Delete dead symlinks
    for f in os.listdir(dest):
        full = os.path.join(dest,f)
        if os.path.islink(full) and not os.path.isfile(full):
            os.unlink(full)

    return 0

if __name__ == '__main__':
    sys.exit(main(sys.argv))
