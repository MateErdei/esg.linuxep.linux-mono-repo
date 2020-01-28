#!/bin/env python

import os
import sys
import shutil
import filecmp
import re
import errno

DIGIT_EXT = re.compile(r"^(.*?)\.\d+$")


def safe_symlink(source, target):
    try:
        os.symlink(source,target)
    except EnvironmentError as e:
        if e.errno == 17:
            return
        print(e)


def safe_delete(p):
    try:
        os.unlink(p)
    except EnvironmentError as e:
        if e.errno == errno.ENOENT:
            return
        raise


def safe_hardlink(source, target):
    safe_delete(target)
    try:
        os.link(source, target)
    except OSError:
        shutil.copy(source, target)


def main(argv):
    dest = argv[1]
    targets = argv[2:]

    touchedFiles = []

    for t in targets:
        assert os.path.isfile(t), "target %s doesn't exist or is not a file" % t
        base = os.path.basename(t)
        target_dest = os.path.join(dest, base)
        touchedFiles.append(base)
        # Copy if changed
        if not (os.path.isfile(target_dest) and filecmp.cmp(t, target_dest)):
            shutil.copy2(t, target_dest)
        # Create symlinks
        target = base
        while True:
            mo = DIGIT_EXT.match(target)
            if not mo:
                break
            # print(target,mo.group(1))
            safe_hardlink(target_dest, os.path.join(dest, mo.group(1)))
            target = mo.group(1)

    # Delete dead symlinks
    for f in os.listdir(dest):
        full = os.path.join(dest, f)
        if os.path.islink(full) and not os.path.isfile(full):
            os.unlink(full)

    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
