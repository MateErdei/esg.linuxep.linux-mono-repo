#!/usr/bin/env python
# Copyright (C) 2019 Sophos Plc, Oxford, England.
# All rights reserved.

from __future__ import absolute_import, print_function, division, unicode_literals

import sys
import os


def readAutoVersion(base_path, jenkins_file):
    assert os.path.isfile(os.path.join(base_path, jenkins_file))
    autoVersionFile = os.path.join(base_path, "products" "distribution" "include", "AutoVersioningHeaders", "AutoVersion.ini")

    if os.path.isfile(autoVersionFile):
        print ("Reading version from {}".format(autoVersionFile), file=sys.stderr)
        with open(autoVersionFile, "r") as f:
            for line in f.readlines():
                if "ComponentAutoVersion=" in line:
                    return line.strip().split("=")[1]

    return None


def main(argv):
    base_path = argv[1]
    base_version = argv[2]
    jenkins_file_path = argv[3]
    ## Get AutoVersion
    autoVersion = readAutoVersion(base_path, jenkins_file_path)

    if autoVersion:
        parts = autoVersion.split(".")
        version = base_version+"."+parts[-1]
    else:
        version = base_version+".999"

    print(version)
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
