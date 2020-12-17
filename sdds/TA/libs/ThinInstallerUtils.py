#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (C) 2020 Sophos Plc, Oxford, England.
# All rights reserved.

import json
import os
import tarfile

try:
    from robot.api import logger
except:
    import logging
    logger = logging.getLogger(__name__)


def build_thininstaller_from_sections(credentials_file, main_thin_installer_source, target_path):
    contents = open(main_thin_installer_source, "rb").read()
    creds = open(credentials_file, "rb").read()

    contents = contents.replace(b"__MIDDLE_BIT__\n", b"__MIDDLE_BIT__\n" + creds)

    open(target_path, "wb").write(contents)
    os.chmod(target_path, 0o700)


def create_credentials_file(destination, token, url, update_creds, message_relays=None, update_caches=None):
    new_line = '\n'
    with open(destination, 'w') as fh:
        fh.write("TOKEN=" + token + new_line)
        fh.write("URL=" + url + new_line)
        fh.write("UPDATE_CREDENTIALS=" + update_creds + new_line)
        if message_relays:
            fh.write("MESSAGE_RELAYS=" + message_relays + new_line)
        if update_caches:
            fh.write("UPDATE_CACHES=" + update_caches + new_line)
            # fh.write("__UPDATE_CACHE_CERTS__" + new_line)
            # with open(os.path.join(self.https_certs_dir, "root-ca.crt.pem"), "r") as f:
            #     fh.write(f.read())
    with open(destination, 'r') as fh:
        logger.info("Credentials section: {}".format(fh.read()))


def get_thin_installer(basedir):
    latest = os.path.join(basedir, "latest.json")
    assert os.path.isfile(latest)
    data = json.load(open(latest))
    latest = data['name']

    tarfile_path = os.path.join(basedir, latest+".tar.gz")

    with tarfile.open(tarfile_path) as tf:
        for tarinfo in tf:
            if tarinfo.name == "SophosSetup.sh":
                f = tf.extractfile(tarinfo)
                return f.read()


def extract_thin_installer(basedir, destination):
    contents = get_thin_installer(basedir)
    with open(destination, "wb") as f:
        f.write(contents)


