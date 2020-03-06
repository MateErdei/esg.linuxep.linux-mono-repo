#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2020 Sophos Plc, Oxford, England.
# All rights reserved.

import glob
import os
import re
import shutil

if __name__ == '__main__':

    capnp_files_origin_dir = "modules/scan_messages"
    capnp_files_target_dir = "output/test-resources/capnp-files"

    os.makedirs(capnp_files_target_dir)

    # Copy capnp files
    capnp_origin_filenames = glob.iglob(f"{capnp_files_origin_dir}/*.capnp")

    for capnp_file in capnp_origin_filenames:
        shutil.copy(capnp_file, capnp_files_target_dir)

    # Remove namespacing
    import_pattern = re.compile(r"using\s+Cxx\s*=\s*import\s+\"capnp/c\+\+\.capnp\";\s*\$Cxx\.namespace\(\".*::.*\"\);")

    capnp_target_filenames = glob.iglob(f"{capnp_files_target_dir}/*.capnp")

    for capnp_file in capnp_target_filenames:
        with open(capnp_file, 'r') as f:
            contents = f.read()
        new_contents = import_pattern.sub(repl="", string=contents, count=1)
        with open(capnp_file, 'w') as f:
            f.write(new_contents)
