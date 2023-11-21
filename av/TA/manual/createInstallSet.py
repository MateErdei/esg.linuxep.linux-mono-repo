#!/usr/bin/env python3
from __future__ import absolute_import, print_function, division, unicode_literals
# Create an install set from the latest supplements from Artifactory, and SDDS-COMPONENT

import filecmp
import os
import shutil
import sys

import downloadSupplements
ensure_binary = downloadSupplements.ensure_binary
ensure_unicode = downloadSupplements.ensure_unicode

LOGGER = None


class Args:
    pass


def log(*x):
    global LOGGER
    if LOGGER is None:
        print(*x)
    else:
        # noinspection PyUnresolvedReferences
        LOGGER.info(" ".join(x))


def download_supplements(dest):
    # ensure manual dir is on sys.path
    downloadSupplements.LOGGER = LOGGER

    args = Args()
    args.arm64 = True
    updated = downloadSupplements.run(ensure_unicode(dest), args)
    return updated


def susi_update_source_dir(install_set):
    install_set = ensure_binary(install_set)
    return os.path.join(install_set,
                        b"files", b"plugins", b"av", b"chroot", b"susi", b"update_source")


def is_same(p1, p2):
    try:
        return filecmp.cmp(p1, p2, False)
    except EnvironmentError:
        return False


def verify_install_set(install_set, sdds_component=None):
    install_set = ensure_binary(install_set)
    if not os.path.isdir(install_set):
        return False

    update_source = susi_update_source_dir(install_set)
    for x in b"vdl", b"model", b"reputation":
        p = os.path.join(update_source, x)
        log("Checking if %s exists" % p)
        if not os.path.isdir(p):
            return False

    # Compare manifest.dat inside sdds_component
    if sdds_component is not None:
        sdds_component = ensure_binary(sdds_component)
        if os.path.isdir(sdds_component):
            sdds_manifest = os.path.join(sdds_component, b"manifest.dat")
            install_manifest = os.path.join(install_set, b"manifest.dat")
            if not is_same(sdds_manifest, install_manifest):
                return False

    return True


def setup_install_set(install_set, sdds_component, vdl, ml_model, local_rep):
    # Copy SDDS-Component
    shutil.rmtree(install_set, ignore_errors=True)
    shutil.copytree(sdds_component, install_set)

    update_source = susi_update_source_dir(install_set)
    shutil.copytree(vdl, os.path.join(update_source, b"vdl"))
    shutil.copytree(ml_model, os.path.join(update_source, b"model"))
    shutil.copytree(local_rep, os.path.join(update_source, b"reputation"))

    return 0


def create_install_set(install_set, sdds_component, base):
    vdl = os.path.join(base, b"dataseta")
    ml_model = os.path.join(base, b"ml_model")
    lrdata = os.path.join(base, b"local_rep")
    return setup_install_set(install_set, sdds_component, vdl, ml_model, lrdata)


def create_install_set_if_required(install_set, sdds_component, base):
    updated = download_supplements(base)
    log("Finished downloading supplements:", str(updated))
    if verify_install_set(install_set, sdds_component) and not updated:
        return 0
    log("Creating install set")
    return create_install_set(install_set, sdds_component, ensure_binary(base))


def main(argv):
    # Delete install set if you want it recreated
    # Recreate install set if sdds_component is different
    if len(argv) == 4:
        install_set = ensure_binary(argv[1])
        sdds_component = ensure_binary(argv[2])
        base = argv[3]
        return create_install_set_if_required(install_set, sdds_component, base)
    elif len(argv) == 1:
        # /opt/test/inputs/av/INSTALL-SET /opt/test/inputs/av/SDDS-COMPONENT /opt/test/inputs/av/..
        install_set = ensure_binary("/opt/test/inputs/av/INSTALL-SET")
        sdds_component = ensure_binary("/opt/test/inputs/av/SDDS-COMPONENT")
        base = "/opt/test/inputs"
        return create_install_set_if_required(install_set, sdds_component, base)

    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv))
