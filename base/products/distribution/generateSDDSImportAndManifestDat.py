#!/usr/bin/env python

from __future__ import absolute_import, print_function, division, unicode_literals

import fileInfo
import generateManifestDat
import generateSDDSImport

import os
import sys


def main(argv):
    dist = argv[1]
    if len(argv) > 2:
        distribution_list = argv[2]
    else:
        distribution_list = None
    if len(argv) > 3:
        BASE = argv[3]
    else:
        BASE = os.environ.get("BASE", None)

    file_objects = fileInfo.load_file_info(dist, distribution_list)

    changed = generateManifestDat.generate_manifest(dist, file_objects)

    sdds_import_path = os.path.join(dist, b"SDDS-Import.xml")
    if os.path.isfile(sdds_import_path) and not changed:
        print("Contents not changed: not regenerating manifest.dat and SDDS-Import.xml")
        return 0

    ## Add manifest.dat to file_list
    file_objects.append(fileInfo.FileInfo(dist, b"manifest.dat"))

    generateSDDSImport.generate_sdds_import(dist, file_objects, BASE)

    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
