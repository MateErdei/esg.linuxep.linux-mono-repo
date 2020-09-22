#!/usr/bin/env python3

import filecmp
import os
import shutil
import sys
from distutils.core import setup

import downloadSupplements
ensure_binary = downloadSupplements.ensure_binary

LOGGER = None

def log(*x):
    global LOGGER
    if LOGGER is None:
        print(*x)
    else:
        LOGGER.info(" ".join(x))


def download_supplements(dest):
    # ensure manual dir is on sys.path
    downloadSupplements.LOGGER = LOGGER
    ret = downloadSupplements.run(dest)
    assert ret == 0


def susi_dir(install_set):
    return os.path.join(install_set,
                        b"files", b"plugins", b"av", b"chroot", b"susi", b"distribution_version", b"version1")


def is_same(p1, p2):
    try:
        return filecmp.cmp(p1, p2, False)
    except EnvironmentError:
        return False


def verify_install_set(install_set, sdds_component=None):
    if not os.path.isdir(install_set):
        return False

    dest = susi_dir(install_set)
    for x in b"vdb", b"mlmodel", b"lrdata":
        if not os.path.isdir(os.path.join(dest, x)):
            return False

    # Compare manifest.dat inside sdds_component
    if sdds_component is not None:
        sdds_manifest = os.path.join(sdds_component, b"manifest.dat")
        install_manifest = os.path.join(install_set, b"manifest.dat")
        if not is_same(sdds_manifest, install_manifest):
            return False

    return True


def setup_install_set(install_set, sdds_component, vdl, ml_model, local_rep):
    # Copy SDDS-Component
    shutil.rmtree(install_set, ignore_errors=True)
    shutil.copytree(sdds_component, install_set)

    dest = susi_dir(install_set)
    shutil.copytree(vdl, os.path.join(dest, b"vdb"))
    shutil.copytree(ml_model, os.path.join(dest, b"mlmodel"))
    shutil.copytree(local_rep, os.path.join(dest, b"lrdata"))
    return 0


def create_install_set(install_set, sdds_component, base):
    vdl = os.path.join(base, b"vdl")
    ml_model = os.path.join(base, b"ml_model")
    lrdata = os.path.join(base, b"local_rep")
    download_supplements(base)
    return setup_install_set(install_set, sdds_component, vdl, ml_model, lrdata)


def create_install_set_if_required(install_set, sdds_component, base):
    if verify_install_set(install_set, sdds_component):
        return 0
    return create_install_set(install_set, sdds_component, base)


def main(argv):
    # Delete install set if you want it recreated
    # Recreate install set if sdds_component is different
    if len(argv) == 4:
        install_set = ensure_binary(argv[1])
        sdds_component = ensure_binary(argv[2])
        base = ensure_binary(argv[3])
        return create_install_set_if_required(install_set, sdds_component, base)

    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv))
