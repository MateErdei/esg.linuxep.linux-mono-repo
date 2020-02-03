#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2019-2020 Sophos Plc, Oxford, England.
# All rights reserved.



import os
import shutil
import time
import subprocess
from robot.api import logger
from robot.libraries.BuiltIn import BuiltIn, RobotNotRunningError


def get_sspl_audit_binary_name():
    try:
        return format(BuiltIn().get_variable_value("${SSPL_AUDIT_BINARY_NAME}"))
    except RobotNotRunningError:
        return os.environ.get("SSPL_AUDIT_BINARY_NAME", "sophosauditplugin")


def disable_system_auditd():
    if not os.path.exists("/sbin/auditd"):
        return
    ssplAuditBinaryName = get_sspl_audit_binary_name()
    print("disable system auditd")
    if os.path.exists('/usr/bin/yum'):
        logger.info(subprocess.check_output(['/usr/sbin/service', 'auditd', 'stop']))
    else:
        print("disable via systemctl")
        logger.info(subprocess.check_output(['/bin/systemctl', 'stop', 'auditd']))

    for attempt in range(5):
        try:
            output = subprocess.check_output(['/usr/bin/pgrep',"-f", ssplAuditBinaryName])
            logger.info(output)
        except subprocess.CalledProcessError as err:
            # the pgrep will not succeed (generate CalledProcessError) when the process is not found
            # which is a success as the plugin is not to be running
            logger.info(str(err))
            return
        time.sleep(5)
    # FIXME: LINUXEP-7271 the long time at the moment is related to the fact that audit-plugin has been 'slowed down'
    raise AssertionError("AuditPlugin still running, after 25 seconds.")


def clear_auditd_logs():
    auditlog = '/var/log/audit/audit.log'
    try:
        os.remove(auditlog)
    except EnvironmentError as e:
        import errno
        if e.errno == errno.ENOENT:
            return
        raise


def display_auditd_logs():
    audit_log = '/var/log/audit/audit.log'
    if not os.path.exists(audit_log):
        return
    if not os.path.exists('/sbin/ausearch'):
        logger.warn("Ausearch not found. Displaying the content of audit.log")
        logger.log_file(audit_log)
        return
    try:
        output = subprocess.check_output(['/sbin/ausearch', '-i'])
        logger.info(output)
    except subprocess.CalledProcessError as ex:
        logger.warn("Called process error: message {} exit_code {}\n output: {}".format(ex.message, ex.returncode, ex.output))
    except Exception as ex:
        logger.warn(repr(ex))
        raise AssertionError("Failed to display audit logs")


PLUGIN_BINARY_PATH = '/root/record_bin_events'
PLUGIN_DISP_CONFIG_PATH = '/etc/audisp/plugins.d/record_bin_events.conf'
PLUGIN_BINARY_DATA_PATH = '/root/AuditEvents.bin.tmp'

def clear_audit_plugin_collect_binary_data():
    if os.path.exists(PLUGIN_DISP_CONFIG_PATH):
        os.remove(PLUGIN_DISP_CONFIG_PATH)
    if os.path.exists(PLUGIN_BINARY_PATH):
        os.remove(PLUGIN_BINARY_PATH)
    if os.path.exists(PLUGIN_BINARY_DATA_PATH):
        os.remove(PLUGIN_BINARY_DATA_PATH)


def setup_audit_plugin_collect_binary_data():
    clear_audit_plugin_collect_binary_data()
    if not os.path.exists(PLUGIN_BINARY_PATH):
        with open(PLUGIN_BINARY_PATH, 'w+') as filehandler:
            filehandler.write(b'''#!/bin/bash
cat >> {}
'''.format(PLUGIN_BINARY_DATA_PATH))
        os.chmod(PLUGIN_BINARY_PATH, 0o750)
    if not os.path.exists( PLUGIN_DISP_CONFIG_PATH):
        with open(PLUGIN_DISP_CONFIG_PATH, 'w+') as filehandler:
            filehandler.write(b'''active = yes
direction = out
path = {}
type = always
format = binary
'''.format(PLUGIN_BINARY_PATH))
        os.chmod(PLUGIN_DISP_CONFIG_PATH, 0o600)
    if not os.path.exists(PLUGIN_BINARY_PATH):
        raise AssertionError("Failed to create file: {}".format(PLUGIN_BINARY_PATH))



def store_binary_data(target_path):
    logger.info("Called store binary data with: {}".format(target_path))
    if os.path.exists(PLUGIN_BINARY_DATA_PATH):
        dir_path, file_name = os.path.split(target_path)
        if dir_path == "":
            dir_path = 'failedTests'
            if not os.path.exists(dir_path):
                os.mkdir(dir_path)
        target_path = os.path.join(dir_path, file_name)
        logger.info("Record the binary file to {} ".format(target_path))
        shutil.move(PLUGIN_BINARY_DATA_PATH, target_path)



def _uninstall_script_content():
    if os.path.exists('/usr/bin/yum'):
        return """#!/bin/bash
service audit stop
yum remove -y audit
rm -rf /etc/audit/*
rm -rf /etc/audisp/*
"""
    else:
        return """#!/bin/bash
systemctl stop auditd
apt-get remove --purge -y auditd

rm -rf /etc/audit/*
rm -rf /etc/audisp/*
"""


def ensure_auditd_is_not_installed():
    if not os.path.exists('/sbin/auditd'):
        return
    tempfile = '/tmp/uninstall_auditd.sh'
    filehandle = open(tempfile, 'w')
    filehandle.write(_uninstall_script_content())
    filehandle.close()

    try:
        output = subprocess.check_output(['/bin/bash', '-x', tempfile])
        logger.info(output)
        if os.path.exists('/sbin/auditd'):
            raise AssertionError("Failed to uninstall auditd")
    finally:
        os.remove(tempfile)


