#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2018-2020 Sophos Plc, Oxford, England.
# All rights reserved.
import grp
import os
import pwd
import shutil
import stat
import subprocess
import tempfile
import time

import robot.libraries.BuiltIn
from robot.api import logger
from robot.libraries.BuiltIn import BuiltIn


def get_variable(var_name, default_value=None):
    try:
        return BuiltIn().get_variable_value("${%s}" % var_name)
    except robot.libraries.BuiltIn.RobotNotRunningError:
        return os.environ.get(var_name, default_value)


def run_proc_with_safe_output(args):
    logger.debug('Run Command: {}'.format(args))
    with tempfile.TemporaryFile(dir=os.path.abspath('.')) as tmpfile:
        p = subprocess.Popen(args, stdout=tmpfile, stderr=tmpfile)
        p.wait()
        tmpfile.seek(0)
        output = tmpfile.read().decode()
        return output, p.returncode


def unmount_all_comms_component_folders(skip_stop_proc=False):
    def _umount_path(fullpath):
        stdout, code = run_proc_with_safe_output(['umount', fullpath])
        if 'not mounted' in stdout:
            return
        if code != 0:
            logger.info(stdout)

    def _stop_commscomponent():
        stdout, code = run_proc_with_safe_output(["/opt/sophos-spl/bin/wdctl", "stop", "commscomponent"])
        if code != 0 and not 'Watchdog is not running' in stdout:
            logger.info(stdout)

    if not os.path.exists('/opt/sophos-spl/bin/wdctl'):
        return
    # stop the comms component as it could be holding the mounted paths and
    # would not allow them to be unmounted.
    counter = 0
    while not skip_stop_proc and counter < 5:
        counter = counter + 1
        stdout, errcode = run_proc_with_safe_output(['pidof', 'CommsComponent'])
        if errcode == 0:
            logger.info("Commscomponent running {}".format(stdout))
            _stop_commscomponent()
            time.sleep(1)
        else:
            logger.info("Skip stop comms componenent")
            break

    dirpath = '/opt/sophos-spl/var/sophos-spl-comms/'

    mounted_entries = ['etc/resolv.conf', 'etc/hosts', 'usr/lib', 'usr/lib64', 'lib',
                       'etc/ssl/certs', 'etc/pki/tls/certs', 'etc/pki/ca-trust/extracted', 'base/mcs/certs',
                       'base/remote-diagnose/output']
    for entry in mounted_entries:
        try:
            fullpath = os.path.join(dirpath, entry)
            if not os.path.exists(fullpath):
                continue
            _umount_path(fullpath)
            if os.path.isfile(fullpath):
                os.remove(fullpath)
            else:
                shutil.rmtree(fullpath)
        except Exception as ex:
            logger.error(str(ex))


def get_group(gid, cache=None):
    if cache is None:
        cache = {}
    if gid in cache:
        return cache[gid]

    try:
        cache[gid] = grp.getgrgid(gid).gr_name
    except KeyError:
        cache[gid] = str(gid)

    return cache[gid]


def get_user(uid, cache=None):
    if cache is None:
        cache = {}
    if uid in cache:
        return cache[uid]

    try:
        cache[uid] = pwd.getpwuid(uid).pw_name
    except KeyError:
        cache[uid] = str(uid)

    return cache[uid]


def get_result_and_stat(path):
    def get_perms(mode):
        p = oct(mode & 0o1777)[-4:]
        if p[0] == '0':
            p = p[-3:]
        return p

    s = os.lstat(path)
    result = "{}, {}, {}, {}".format(get_perms(s.st_mode), get_group(s.st_gid), get_user(s.st_uid), path)
    return result, s


def get_file_info_for_installation():
    """
    ${FileInfo}=  Run Process  find  ${SOPHOS_INSTALL}  -type  f
    Should Be Equal As Integers  ${FileInfo.rc}  0  Command to find all files and permissions in install dir failed: ${FileInfo.stderr}
    Create File    ./tmp/NewFileInfo  ${FileInfo.stdout}
    ${FileInfo}=  Run Process  grep  -vFf  tests/installer/InstallSet/ExcludeFiles  ./tmp/NewFileInfo
    Create File    ./tmp/NewFileInfo  ${FileInfo.stdout}
    ${FileInfo}=  Run Process  cat  ./tmp/NewFileInfo  |  xargs  stat  -c  %a, %G, %U, %n  shell=True
    Create File    ./tmp/NewFileInfo  ${FileInfo.stdout}
    ${FileInfo}=  Run Process  sort  ./tmp/NewFileInfo

    And similar for directories and symlinks

    :return:
    """
    sophos_install = os.path.join(get_variable("SOPHOS_INSTALL"), "plugins/edr")
    exclusions = open(os.path.join(get_variable("INSTALL_SET_PATH"), "ExcludeFiles")).readlines()
    exclusions = set((e.strip() for e in exclusions))

    full_files = set()
    full_directories = [sophos_install]

    for (base, dirs, files) in os.walk(sophos_install):
        for f in files:
            include = True
            for e in exclusions:
                if e in f or e in base:
                    include = False
                    break
            if include:
                full_files.add(os.path.join(base, f))
        for d in dirs:
            include = True
            for e in exclusions:
                if e in d or e in base:
                    include = False
                    break
            if include:
                full_directories.append(os.path.join(base, d))

    details = []
    symlinks = []
    unixsocket = []

    for f in full_files:
        result, s = get_result_and_stat(f)
        if stat.S_ISLNK(s.st_mode):
            if not result.endswith('pyc'):
                symlinks.append(result)
        elif stat.S_ISSOCK(s.st_mode):
            unixsocket.append(result)
        else:
            if not result.endswith('pyc.0'):
                details.append(result)

    key = lambda x: x.replace(".", "").lower()
    symlinks.sort(key=key)
    details.sort(key=key)

    directories = []
    for d in full_directories:
        result, s = get_result_and_stat(d)
        directories.append(result)
    directories.sort(key=key)

    return "\n".join(directories), "\n".join(details), "\n".join(symlinks)


def get_systemd_file_info():
    """

    ${SystemdPath}  Run Keyword If  os.path.isdir("/lib/systemd/system/")  Set Variable   /lib/systemd/system/
    ...  ELSE  Set Variable   /usr/lib/systemd/system/
    ${SystemdInfo}=  Run Process  find  ${SystemdPath}  -name  sophos-spl*  -type  f  -exec  stat  -c  %a, %G, %U, %n  {}  +
    Should Be Equal As Integers  ${SystemdInfo.rc}  0  Command to find all systemd files and permissions failed: ${SystemdInfo.stderr}
    Create File  ./tmp/NewSystemdInfo  ${SystemdInfo.stdout}
    ${SystemdInfo}=  Run Process  sort  ./tmp/NewSystemdInfo

    :return:
    """

    systemd_path = "/usr/lib/systemd/system/"
    if os.path.isdir("/lib/systemd/system/"):
        systemd_path = "/lib/systemd/system/"

    full_files = []

    for (base, dirs, files) in os.walk(systemd_path):
        for f in files:
            if not f.startswith("sophos-spl"):
                continue
            p = os.path.join(base, f)
            if os.path.islink(p):
                continue
            full_files.append(p)

    results = []
    for p in full_files:
        result, s = get_result_and_stat(p)
        results.append(result)
    key = lambda x: x.replace(".", "").replace("-", "").lower()
    results.sort(key=key)

    return "\n".join(results)
