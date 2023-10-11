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
    logger = logging.getLogger("BaseUtils")

def get_variable(varName, defaultValue=None):
    try:
        return BuiltIn().get_variable_value("${}".format(varName))
    except RobotNotRunningError:
        return os.environ.get(varName, defaultValue)


def uninstall_sspl_if_installed():
    """
    Calls /opt/sophos-spl/bin/uninstall.sh if present


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

    dot_sophos = os.path.join(SOPHOS_INSTALL, ".sophos")
    if not os.path.isfile(dot_sophos):
        logger.warning("/opt/sophos-spl/.sophos doesn't exist - uninstaller would break")
        open(dot_sophos, "wb").close()

    p = subprocess.run(["bash", uninstaller, "--force"], timeout=60, stderr=subprocess.PIPE, stdout=subprocess.STDOUT)
    if p.returncode != 0:
        logger.warning(p.stdout)
        raise Exception("Failed to uninstall")

def create_test_telemetry_config_file(telemetry_config_file_path, certificate_path, username="sophos-spl-user",
                                      requestType="PUT", port=443):
    default_telemetry_config = {
        "telemetryServerCertificatePath": certificate_path,
        "externalProcessWaitRetries": 10,
        "externalProcessWaitTime": 100,
        "additionalHeaders": {
            "x-amz-acl": "bucket-owner-full-control"
        },
        "maxJsonSize": 100000,
        "messageRelays": [],
        "port": int(port),
        "proxies": [],
        "resourcePath": "linux/dev",
        "server": "localhost",
        "verb": requestType
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


def install_base_if_not_installed():
    SOPHOS_INSTALL = get_variable("SOPHOS_INSTALL", "/opt/sophos-spl")
    if not os.path.isdir(SOPHOS_INSTALL):
        BuiltIn().run_keyword("Install Base For Component Tests")
        return 0


def set_spl_log_level_and_restart_watchdog_if_changed(loglevel: str):
    """

    Create File  ${SOPHOS_INSTALL}/base/etc/logger.conf.local  [global]\nVERBOSITY=${logLevel}

    :param loglevel:
    :return:
    """
    SOPHOS_INSTALL = get_variable("SOPHOS_INSTALL", "/opt/sophos-spl")
    etc = os.path.join(SOPHOS_INSTALL, "base", "etc")
    conf = os.path.join(etc, "logger.conf.local")
    expected = "[global]\nVERBOSITY="+loglevel
    try:
        existing = open(conf).read().strip()
        if existing == expected.strip():
            logger.info("Log configuration already set")
            return
    except OSError:
        pass

    logger.info("Updating log configuration")
    open(conf, "w").write(expected)
    open(os.path.join(etc, "logger.conf"), "w").write(expected)

    logger.info("Restarting sophos-spl")
    subprocess.check_call(['systemctl', 'restart', 'sophos-spl'])


def clear_outbreak_mode_if_required():
    SOPHOS_INSTALL = get_variable("SOPHOS_INSTALL", "/opt/sophos-spl")
    outbreak_status_path = os.path.join(SOPHOS_INSTALL, "var", "sophosspl", "outbreak_status.json")
    if not os.path.isfile(outbreak_status_path):
        logger.info("DEBUG::: Not in outbreak mode: " + outbreak_status_path)
        return

    logger.info("Clearing outbreak mode")
    wdctl = os.path.join(SOPHOS_INSTALL, "bin", "wdctl")
    subprocess.check_call([wdctl, "stop", "managementagent"])
    os.unlink(outbreak_status_path)
    subprocess.check_call([wdctl, "start", "managementagent"])
