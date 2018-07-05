#!/usr/bin/env python

from __future__ import absolute_import,print_function,division,unicode_literals

import sys

import generateSDDSImport
import generateManifestDat
import fileInfo

def main(argv):
    dist = argv[1]
    file_objects = fileInfo.load_file_info(dist, argv[2])

    generateManifestDat.generate_manifest(dist, file_objects)

    ## Add manifest.dat to file_list
    file_objects.append(fileInfo.FileInfo(dist, b"manifest.dat"))

    generateSDDSImport.generate_sdds_import(dist, file_objects)

    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
