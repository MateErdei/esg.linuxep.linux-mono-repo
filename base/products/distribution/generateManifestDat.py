#!/usr/bin/env python
# Copyright 2019-2023 Sophos Limited. All rights reserved.


import os
import sys
import subprocess

import fileInfo


def read(p):
    try:
        c = open(p, "rb").read()
        return c.split(b"-----BEGIN SIGNATURE-----")[0]
    except EnvironmentError:
        return None



def generate_manifest_new_api(dist):
    MANIFEST_NAME = os.environ.get("MANIFEST_NAME", "manifest.dat")
    manifest_path = os.path.join(dist, MANIFEST_NAME)
    exclusions = 'SDDS-Import.xml,'+MANIFEST_NAME  # comma separated string
    env = os.environ.copy()
    env['LD_LIBRARY_PATH'] = "/usr/lib:/usr/lib64"
    env['OPENSSL_PATH'] = "/usr/bin/openssl"
    previous_contents = read(manifest_path)
    result = subprocess.run(
        ['sb_manifest_sign', '--folder', dist, '--output', manifest_path, '--exclusions', exclusions]
        , stderr=subprocess.STDOUT, stdout=subprocess.PIPE, env=env,
        timeout=60
    )
    result.check_returncode()
    new_contents = read(manifest_path)
    if previous_contents == new_contents:
        #we dont wan't to fail local builds where only test tools have changed
        print("manifest has not changed since last build")
    return True


def generate_manifest(dist):
    try:
        return generate_manifest_new_api(dist)
    except subprocess.CalledProcessError as ex:
        print("Unable to generate manifest.dat file with new-api: ", ex.returncode, str(ex))
        print("Output:", ex.output.decode("UTF-8", errors='replace'))
    except Exception as ex:
        print("Unable to generate manifest.dat file with new-api: ", str(ex))
    return False


def main(argv):
    dist = argv[1]
    if len(argv) > 2:
        distribution_list = argv[2]
    else:
        distribution_list = None

    file_objects = fileInfo.load_file_info(dist, distribution_list)
    generate_manifest(dist, file_objects)

    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))