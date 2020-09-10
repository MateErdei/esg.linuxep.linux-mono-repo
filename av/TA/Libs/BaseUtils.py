#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2020 Sophos Plc, Oxford, England.
# All rights reserved.

import json
import os
import pwd
import shutil
import subprocess

from robot.libraries.BuiltIn import BuiltIn, RobotNotRunningError


try:
    from robot.api import logger
    logger.warning = logger.warn
except ImportError:
    import logging
    logger = logging.getLogger("UpdateServer")

def get_variable(varName, defaultValue=None):
    try:
        return BuiltIn().get_variable_value("${}".format(varName))
    except RobotNotRunningError:
        return os.environ.get(varName, defaultValue)


def uninstall_sspl_if_installed():
    """
    Run Process  bash ${SOPHOS_INSTALL}/bin/uninstall.sh --force   shell=True   timeout=20s


    Fail if the uninstall returns an error.
    """
    SOPHOS_INSTALL = get_variable("SOPHOS_INSTALL", "/opt/sophos-spl")
    if not os.path.isdir(SOPHOS_INSTALL):
        return 0

    uninstaller = os.path.join(SOPHOS_INSTALL, "bin", "uninstall.sh")
    if not os.path.isfile(uninstaller):
        logger.info("{} exists but uninstaller doesn't - removing directory".format(SOPHOS_INSTALL))
        shutil.rmtree(SOPHOS_INSTALL)
        return 0

    subprocess.check_call(["bash", uninstaller, "--force"], timeout=20)


def create_test_telemetry_config_file(telemetry_config_file_path, certificate_path, username="sophos-spl-user",
                                      requestType="PUT", port=443):
    default_telemetry_config = {
        "telemetryServerCertificatePath": "",
        "externalProcessWaitRetries": 10,
        "externalProcessWaitTime": 100,
        "additionalHeaders": ["x-amz-acl: bucket-owner-full-control"],
        "maxJsonSize": 100000,
        "messageRelays": [],
        "port": 443,
        "proxies": [],
        "resourcePath": "linux/dev",
        "server": "localhost",
        "verb": "PUT"
    }

    fixedTelemetryCert="/opt/sophos-spl/base/mcs/certs/telemetry.crt"

    logger.info(certificate_path)

    if certificate_path != '""':
        shutil.copyfile(certificate_path, fixedTelemetryCert)
        default_telemetry_config["telemetryServerCertificatePath"] = fixedTelemetryCert
    else:
        default_telemetry_config["telemetryServerCertificatePath"] = ""

    os.chmod(fixedTelemetryCert, 0o644)
    default_telemetry_config["verb"] = requestType
    default_telemetry_config["port"] = int(port)

    with open(telemetry_config_file_path, 'w') as tcf:
        tcf.write(json.dumps(default_telemetry_config))

    uid = pwd.getpwnam(username).pw_uid
    os.chown(telemetry_config_file_path, uid, -1)

