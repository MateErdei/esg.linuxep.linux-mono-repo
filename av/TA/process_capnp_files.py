#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2020 Sophos Plc, Oxford, England.
# All rights reserved.

import os
import glob
import re

if __name__ == '__main__':
    # TODO Read & Process capnp files
    # if (/resources/capnp-files) doesn't exist
    #   Raise an error
    # for each capnp file in /capnp-files
    #   read
    #   replace
    #   write (in place)

    capnp_files_dir = "/opt/test/inputs/test_scripts/resources/capnp-files"
    import_pattern = \
        re.compile(r"using\s+Cxx\s*=\s*import\s+\"capnp/c\+\+\.capnp\";\s*\$Cxx\.namespace\(\".*::.*\"\);")

    if not os.path.isdir(capnp_files_dir):
        # TODO Make nice error message if this build hasn't run
        raise FileNotFoundError(f"{capnp_files_dir} does not exist, please run build.sh")

    capnp_filenames = glob.iglob(f"{capnp_files_dir}/*.capnp")

    for capnp_file in capnp_filenames:
        with open(capnp_file, 'r') as f:
            contents = f.read()
        new_contents = import_pattern.sub(repl="", string=contents, count=1)
        print(new_contents)
        # TODO Write Capnp Files (to a resources folder)
        with open(capnp_file, 'w') as f:
            f.write(new_contents)

    # TODO Update readme
    # TODO DOCUMENTATION OF WHERE CAPNP FILES ARE COPIED FROM
    #  (IN THE TESTS, SO IF SOMEONE GOES TO THE TESTS, THEY KNOW WHERE THEY'VE GONE WRONG)
