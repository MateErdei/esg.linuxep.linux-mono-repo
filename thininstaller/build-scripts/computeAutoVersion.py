#!/usr/bin/env python
# Copyright (C) 2019 Sophos Plc, Oxford, England.
# All rights reserved.

from __future__ import absolute_import, print_function, division, unicode_literals

import sys
import os


def readAutoVersion():
    BASE = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
    assert os.path.isfile(os.path.join(BASE, "Jenkinsfile"))
    autoVersionFile = os.path.join(BASE, "AutoVersioningHeaders", "AutoVersion.ini")

    if os.path.isfile(autoVersionFile):
        print ("Reading version from {}".format(autoVersionFile), file=sys.stderr)
        with open(autoVersionFile, "r") as f:
            for line in f.readlines():
                if "ComponentAutoVersion=" in line:
                    return line.strip().split("=")[1]

    return None


def main(argv):
    baseVersion = argv[1]

    ## Get AutoVersion
    autoVersion = readAutoVersion()

    if autoVersion:
        parts = autoVersion.split(".")
        version = baseVersion+"."+parts[-1]
    else:
        version = baseVersion+".999"

    print(version)
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))