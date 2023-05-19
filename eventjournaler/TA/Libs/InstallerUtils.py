# Copyright 2023 Sophos Limited. All rights reserved.
import grp
import os
import pwd
import stat
import subprocess

import robot.libraries.BuiltIn
from robot.libraries.BuiltIn import BuiltIn


def get_variable(var_name, default_value=None):
    try:
        return BuiltIn().get_variable_value("${%s}" % var_name)
    except robot.libraries.BuiltIn.RobotNotRunningError:
        return os.environ.get(var_name, default_value)


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
    result = f"{get_perms(s.st_mode)}, {get_group(s.st_gid)}, {get_user(s.st_uid)}, {path}"
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
    sophos_install = os.path.join(get_variable("SOPHOS_INSTALL"), "plugins/eventjournaler")
    exclusions = open(os.path.join(get_variable("ROBOT_SCRIPTS_PATH"), "InstallSet/ExcludeFiles")).readlines()
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


def initial_cleanup():
    dev_null = open("/dev/null", "wb")
    subprocess.call(["userdel", "sophos-spl-av"], stderr=dev_null)
    subprocess.call(["userdel", "sophos-spl-threat-detector"], stderr=dev_null)
    subprocess.call(["groupdel", "sophos-spl-group"], stderr=dev_null)
