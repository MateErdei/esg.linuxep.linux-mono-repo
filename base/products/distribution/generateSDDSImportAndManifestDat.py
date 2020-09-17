#!/usr/bin/env python

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

    if len(argv) > 4:
        sys.path += argv[4:]

    import fileInfo
    import generateManifestDat
    import generateSDDSImport

    generateManifestDat.generate_manifest(dist)

    file_objects = fileInfo.load_file_info(dist, distribution_list, excludeManifest=False)
    # file_objects will already contain manifest.dat, since it is generated after generating the manifest

    generateSDDSImport.generate_sdds_import(dist, file_objects, BASE)

    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
