#!/usr/bin/env python


import fileInfo
import generateManifestDat
import generateSDDSImport

import os
import sys
import subprocess


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

    # file_objects = fileInfo.load_file_info(dist, distribution_list)

    exclusions = 'SDDS-Import.xml,manifest.dat'  # comma separated string
    env = dict(os.environ)
    env['LD_LIBRARY_PATH'] = "/usr/lib:/usr/lib64"
    proc = subprocess.Popen(
        ['sb_manifest_sign', '--folder', f'{dist}', '--output', f'{dist}/manifest.dat', '--exclusions', f'{exclusions}']
        , stderr=subprocess.PIPE, stdout=subprocess.PIPE, env=env)
    try:
        outs, errs = proc.communicate(timeout=15)
    except subprocess.TimeoutExpired:
        proc.kill()
        outs, errs = proc.communicate()

    if proc.returncode != 0:
        raise AssertionError(errs)

    ## Add manifest.dat to file_list
    file_objects = fileInfo.load_file_info(dist, distribution_list)
    file_objects.append(fileInfo.FileInfo(dist, "manifest.dat"))

    generateSDDSImport.generate_sdds_import(dist, file_objects, BASE)

    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
