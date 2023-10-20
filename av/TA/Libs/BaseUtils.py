#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright 2020-2023 Sophos Limited. All rights reserved.
# All rights reserved.

import grp
import json
import os
import pwd
import re
import shutil
import subprocess
import sys

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


def user_exists(username: str):
    try:
        pwd.getpwnam(username)
        return True
    except KeyError:
        return False


def group_exists(groupname: str):
    try:
        grp.getgrnam(groupname)
        return True
    except KeyError:
        return False


def delete_av_users():
    for user in ('sophos-spl-av', 'sophos-spl-threat-detector'):
        if user_exists(user):
            subprocess.run(['userdel', user])


def delete_users_and_groups():
    delete_av_users()


def _delete_all_users_and_groups():
    delete_av_users()
    if user_exists('sophos-spl-user'):
        subprocess.run(['userdel', 'sophos-spl-user'])
    if group_exists('sophos-spl-group'):
        subprocess.run(['groupdel', 'sophos-spl-group'])


def uninstall_sspl_if_installed():
    """
    Calls /opt/sophos-spl/bin/uninstall.sh if present

    Fail if the uninstall returns an error.
    """
    SOPHOS_INSTALL = get_variable("SOPHOS_INSTALL", "/opt/sophos-spl")
    if not os.path.isdir(SOPHOS_INSTALL):
        delete_users_and_groups()
        return 0

    uninstaller = os.path.join(SOPHOS_INSTALL, "bin", "uninstall.sh")
    if not os.path.isfile(uninstaller):
        logger.info("{} exists but uninstaller doesn't - removing directory".format(SOPHOS_INSTALL))
        shutil.rmtree(SOPHOS_INSTALL)
        delete_users_and_groups()
        return 0

    dot_sophos = os.path.join(SOPHOS_INSTALL, ".sophos")
    if not os.path.isfile(dot_sophos):
        logger.warning("/opt/sophos-spl/.sophos doesn't exist - uninstaller would break")
        open(dot_sophos, "wb").close()

    p = subprocess.run(["bash", uninstaller, "--force"], timeout=60, stderr=subprocess.STDOUT, stdout=subprocess.PIPE)
    if p.returncode != 0:
        logger.warning(p.stdout)
        raise Exception("Failed to uninstall")

    delete_users_and_groups()

def uninstall_sspl_from_custom_location_if_installed(install_dir):
    """
    Fail if the uninstall returns an error.
    """
    if not os.path.isdir(install_dir):
        delete_users_and_groups()
        return 0

    uninstaller = os.path.join(install_dir, "bin", "uninstall.sh")
    if not os.path.isfile(uninstaller):
        logger.info(f"{install_dir} exists but uninstaller doesn't - removing directory")
        shutil.rmtree(install_dir)
        delete_users_and_groups()
        return 0

    dot_sophos = os.path.join(install_dir, ".sophos")
    if not os.path.isfile(dot_sophos):
        logger.warning("/opt/sophos-spl/.sophos doesn't exist - uninstaller would break")
        open(dot_sophos, "wb").close()

    p = subprocess.run(["bash", uninstaller, "--force"], timeout=60, stderr=subprocess.STDOUT, stdout=subprocess.PIPE)
    if p.returncode != 0:
        logger.warning(p.stdout)
        raise Exception("Failed to uninstall")

    delete_users_and_groups()


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


def create_library_symlinks(directory):
    LIB_RE = re.compile(r"(.*?)(\.\d+)$")
    for file in os.listdir(directory):
        f = os.path.join(directory, file)
        if os.path.islink(f):
            continue
        if not os.path.isfile(f):
            continue
        while True:
            mo = LIB_RE.match(file)
            if not mo:
                break
            try:
                os.symlink(file, os.path.join(directory, mo.group(1)))
            except EnvironmentError:
                pass
            file = mo.group(1)


def recursive_create_library_symlinks(directory):
    LIB_RE = re.compile(r"^(lib.*\.so.*?)(\.\d+)$")
    for (base, dirs, files) in os.walk(directory):
        for file in files:
            f = os.path.join(base, file)
            if os.path.islink(f):
                continue
            while True:
                mo = LIB_RE.match(file)
                if not mo:
                    break
                try:
                    os.symlink(file, os.path.join(base, mo.group(1)))
                except EnvironmentError:
                    pass
                file = mo.group(1)


def __main(argv):
    src = argv[1]
    recursive_create_library_symlinks(src)
    return 0


if __name__ == "__main__":
    sys.exit(__main(sys.argv))
