#!/usr/bin/env python
# Copyright (C) 2019 Sophos Plc, Oxford, England.
# All rights reserved.



import readVersion

import sys
import os


def readAutoVersion(base_path):
    assert os.path.isfile(os.path.join(base_path))

    autoVersionFile = readVersion.get_valid_auto_version_path(base_path)

    if autoVersionFile is None:
        return None

    if os.path.isfile(autoVersionFile):
        with open(autoVersionFile, "r") as f:
            for line in f.readlines():
                if "ComponentAutoVersion=" in line:
                    return line.strip().split("=")[1]

    return None


def main(argv):
    base_path = argv[1]
    base_version = argv[2]
    ## Get AutoVersion
    autoVersion = readAutoVersion(base_path)

    if autoVersion:
        version = autoVersion
    else:
        version = base_version+".999"

    print(version)
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
