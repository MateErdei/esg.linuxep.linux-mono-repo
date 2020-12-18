#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (C) 2020 Sophos Plc, Oxford, England.
# All rights reserved.

import os
import subprocess

try:
    from . import PathManager
except ImportError:
    import PathManager

try:
    from robot.api import logger
except:
    import logging
    logger = logging.getLogger(__name__)


def install_system_ca_cert(certificate_path):
    return install_system_ca_certs(certificate_path)


def install_system_ca_certs(*certificate_paths):
    script = os.path.join(PathManager.get_support_file_path(), "https", "InstallCertificateToSystem.sh")
    command = ["bash", script, *certificate_paths]
    logger.debug(command)
    process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    out, _ = process.communicate()
    out = out.decode("UTF-8")
    if process.returncode != 0:
        logger.error("returncode: {}\noutput: {}".format(process.returncode, out))
        raise OSError("Failed to install \"{}\" to system".format(certificate_paths))
    else:
        logger.debug("returncode: {}\noutput: {}".format(process.returncode, out))


def cleanup_system_ca_certs():
    support_files_path = PathManager.get_support_file_path()
    script = os.path.join("bash", support_files_path, "https", "CleanupInstalledSystemCerts.sh")
    logger.debug(script)
    command = [script]
    process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    out, _ = process.communicate()
    out = out.decode("UTF-8")
    if process.returncode != 0:
        logger.error("returncode: {}\noutput: {}".format(process.returncode, out))
        raise OSError("Failed to cleanup system certs")
    else:
        logger.debug("returncode: {}\noutput: {}".format(process.returncode, out))

