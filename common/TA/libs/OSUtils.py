#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2019-2023 Sophos Plc, Oxford, England.
# All rights reserved.


import os
import socket
import stat
import glob
import grp
import pwd
import signal
import time
import subprocess
import shutil
import tarfile
import psutil
import platform
import re

from robot.api import logger

import PathManager


def ensure_chmod_file_matches(filepath, expected_chmod_string):
    given_cmod = oct(os.stat(filepath)[stat.ST_MODE])[-3:]
    if given_cmod != expected_chmod_string:
        raise AssertionError(f"Filepath {filepath} chmod is {given_cmod}. Expected {expected_chmod_string}")


def ensure_owner_and_group_matches(filepath, username, groupname):
    stat_info = os.stat(filepath)
    user = pwd.getpwuid(stat_info.st_uid)[0]
    group = grp.getgrgid(stat_info.st_gid)[0]
    if username != user:
        raise AssertionError(f"Filepath {filepath} owner is {user}. Expected {username}.")
    if group != groupname:
        raise AssertionError(f"Filepath {filepath} belongs to group {group}. Expected {groupname}")


def get_process_owner(pid) -> str:
    proc_stat_file = os.stat(f"/proc/{pid}")
    uid = proc_stat_file.st_uid
    return pwd.getpwuid(uid)[0]

def get_process_group(pid) -> str:
    proc_stat_file = os.stat(f"/proc/{pid}")
    gid = proc_stat_file.st_gid
    return grp.getgrgid(gid).gr_name

def pids_of_file(file_path):
    if not os.path.exists(file_path):
        raise AssertionError(f"Not a valid path: {file_path}")
    pids_of = []

    try:
        output = subprocess.check_output(['pidof', file_path])
        output = output.decode()
        if len(output):
            pids_of = [int(p) for p in output[:-1].split(' ')]
            logger.info(f"Pids found: {pids_of}")
    except subprocess.CalledProcessError as ex:
        # this is expected, as the process was executed but could 
        # not find any running process for the path
        # hence no process to kill
        logger.debug(repr(ex))  # for debug purpose only
        pass

    except Exception as ex:
        # any other exception is not expected
        raise
    return pids_of


def check_if_process_has_osquery_netlink(file_path):
    pids = pids_of_file(file_path)

    if len(pids) < 2:
        raise AssertionError(f"Not all osquery processes are running, only {len(pids)} processes running")
    try:
        output = subprocess.check_output(['auditctl', '-s'])
        string_to_check = output.decode()

    except subprocess.CalledProcessError as ex:
        logger.debug(ex)

    contains = False
    for process_id in pids:
        if str(process_id) in string_to_check:
            contains = True

    return contains


def check_process_running(file_path, expect='Running'):
    """
    file_path path of the file
    expect= Running or NotRunning    
    """
    if expect not in ['Running', 'NotRunning']:
        raise AssertionError(f'Expect option not supported: {expect}')
    pids_of = pids_of_file(file_path)
    if pids_of and expect == 'Running':
        return []
    if pids_of and expect == 'NotRunning':
        raise AssertionError(f"Process still running {file_path} with the pids: {pids_of}")
    if not pids_of and expect == 'NotRunning':
        return []
    if not pids_of and expect == 'Running':
        raise AssertionError(f"No Process running with the path: {file_path}")
    return pids_of


def kill_by_file(file_path):
    pids_of = pids_of_file(file_path)
    if pids_of:

        for pid in pids_of:
            logger.info(f"Issuing kill command to pid: {pid}")
            os.kill(int(pid), signal.SIGTERM)
    else:
        logger.info("No process to kill")
    return pids_of


def kill_by_command(command):
    logger.info(f"Attempt to kill process by command: {command}")
    if os.path.exists(command):
        kill_by_file(command)
    try:
        # remove the new line at the end
        file_path = subprocess.check_output(['which', command])[:-1]

        logger.info(f'Command path: {file_path}')
    except Exception as ex:
        logger.info(repr(ex))
        raise AssertionError(f"Command path not found: {command}")
    kill_by_file(file_path)


def kill_by_file_and_wait_it_to_shutdown(file_path, secs2wait, number_of_attempts):
    pids_before_kill = set(kill_by_file(file_path))
    logger.info(f"Sent the signal kill to processes: {pids_before_kill}")
    number_of_attempts = int(number_of_attempts)
    secs2wait = int(secs2wait)
    attempt = 0
    while attempt < number_of_attempts:
        attempt += 1
        time.sleep(secs2wait)
        pids_after_kill = set(pids_of_file(file_path))
        logger.info(f"Current processes running: {pids_after_kill}")
        if len(pids_before_kill.intersection(pids_after_kill)) == 0:
            return

    raise AssertionError(f"Process not killed after {secs2wait * number_of_attempts} seconds")


def dump_all_processes():
    pstree = '/usr/bin/pstree'
    if os.path.isfile(pstree):
        logger.info(subprocess.check_output([pstree, '-ap'], stderr=subprocess.PIPE))
    else:
        logger.info(subprocess.check_output(['/bin/ps', '-elf'], stderr=subprocess.PIPE))
    try:
        top_path = [candidate for candidate in ['bin/top', '/usr/bin/top'] if os.path.isfile(candidate)][0]
        logger.info(subprocess.check_output([top_path, '-bHn1', '-o', '+%CPU', '-o', '+%MEM']))
    except Exception as ex:
        logger.warn(ex.message)


def remove_dir_if_exists(dir):
    if os.path.isdir(dir):
        logger.info(f"Removing dir: {dir}")
        shutil.rmtree(dir)


def remove_files_in_directory(directory_path):
    for fname in os.listdir(directory_path):
        if fname in ['.', '..']:
            continue
        full_path = os.path.join(directory_path, fname)
        if os.path.isfile(full_path):
            try:
                os.remove(full_path)
            except IOError:
                pass

def replace_string_in_file(old_string, new_string, file):
    if not os.path.isfile(file):
        raise AssertionError(f"File not found {file}")
    with open(file, "rt") as f:
        content = f.read()
    content = content.replace(old_string, new_string)
    with open(file, "wt") as f:
        f.write(content)

def check_files_have_not_been_removed(install_directory, removed_file_list, root_directory_to_check,
                                      files_to_ignore_in_check):
    # function used to check that files have not been removed by the installation cleanup processes even if
    # the files are listed in the remove_files_list created by the manifest diff
    # when the files listed are outside of the realm of the component file set.

    # install_directory: location of sophos-spl
    # removed_file_list: file path of the file created by the manifest diff named removedFiles_manifest files in this list should be prevented from being deleted
    # root_directory_to_check: component root direcotry
    # files_to_ignore_in_check: file path to the files to delete dat file.  These files will be correctly deteted by the correct component
    #   For example if test is checking plugin does not delete base files, the filestodelete.dat file should be the one for base.

    if os.path.isfile(removed_file_list):

        path_to_remove = "files/"

        with open(removed_file_list, "r") as removed_input_file:
            for removed_input_line in removed_input_file:
                removed_input_line = removed_input_line.strip()
                if root_directory_to_check in removed_input_line:
                    full_path = os.path.join(install_directory, removed_input_line[len(path_to_remove):])

                    if not removed_input_line.startswith(path_to_remove):
                        full_path = os.path.join(install_directory, removed_input_line)

                    if os.path.exists(full_path):
                        continue
                    else:
                        with open(files_to_ignore_in_check, "r") as files_to_ignore_in_check_file:
                            ignore_file = False
                            for ignore_input_line in files_to_ignore_in_check_file:
                                ignore_input_line = ignore_input_line.strip()
                                ignore_input_line = ignore_input_line.replace('*', '')
                                if ignore_input_line in full_path:
                                    ignore_file = True
                                    break
                            if ignore_file == False:
                                raise AssertionError(
                                    f"A file has been removed that should have been prevented from being deleted: {full_path}")


def Create_Symlink(target, destination):
    os.symlink(target, destination)


def get_build_type():
    return os.getenv('BUILD_TYPE', "dev")


def query_tarfile_for_contents(path_to_tarfile):
    tar = tarfile.open(path_to_tarfile)
    members = tar.getmembers()
    member_names = []
    for member in members:
        member_names.append(member.name)
    return "\n".join(member_names)


def get_file_owner(filepath: str) -> str:
    owner = pwd.getpwuid(os.stat(filepath).st_uid).pw_name
    return owner


def get_file_owner_id(filepath: str) -> str:
    owner_id = os.stat(filepath).st_uid
    return str(owner_id)


def get_file_group(filepath: str) -> str:
    group = grp.getgrgid(os.stat(filepath).st_gid).gr_name
    return group


def get_file_group_id(filepath: str) -> str:
    group_id = os.stat(filepath).st_gid
    return str(group_id)


def get_file_permissions(filepath):
    perms = subprocess.check_output(['stat', '-c', '%A', filepath]).decode().strip()
    return perms


def get_all_permissions(filepath):
    permissions = []
    permissions.append(get_file_owner(filepath))
    permissions.append(get_file_group(filepath))
    permissions.append(get_file_permissions(filepath))
    return permissions


def check_owner_matches(expected_owner, filepath):
    actual_owner = get_file_owner(filepath)
    assert actual_owner == expected_owner, f"expected owner: {expected_owner}, doesn't match actual: {actual_owner}"


def check_group_matches(expected_group, filepath):
    actual_group = get_file_group(filepath)
    assert actual_group == expected_group, f"expected group: {expected_group}, doesn't match actual: {actual_group}"


def check_file_permissions_match(expected_perms, filepath):
    actual_perms = get_file_permissions(filepath)
    assert actual_perms == expected_perms, f"expected file permissions: {expected_perms}, don't match actual: {actual_perms}"


def file_exists_with_permissions(path_to_file, owner, group, permissions):
    assert os.path.isfile(path_to_file), f"{path_to_file} is not a file"
    check_owner_matches(owner, path_to_file)
    check_group_matches(group, path_to_file)
    check_file_permissions_match(permissions, path_to_file)


def socket_exists_with_permissions(path_to_socket, owner, group, permissions):
    mode = os.stat(path_to_socket).st_mode
    assert stat.S_ISSOCK(mode), f"{path_to_socket} is not a socket"
    check_owner_matches(owner, path_to_socket)
    check_group_matches(group, path_to_socket)
    check_file_permissions_match(permissions, path_to_socket)


def get_dictionary_of_actual_sockets_and_permissions(install_dir="/opt/sophos-spl"):
    dictionary_of_sockets = {}

    base_ipc_files = glob.glob(f"{install_dir}/var/ipc/*.ipc")
    plugin_ipc_files = glob.glob(f"{install_dir}/var/ipc/plugins/*.ipc")

    all_ipc_files = base_ipc_files + plugin_ipc_files

    for path_to_socket in all_ipc_files:
        mode = os.stat(path_to_socket).st_mode
        assert stat.S_ISSOCK(mode), f"{path_to_socket} is not a socket"

        permissions = get_all_permissions(path_to_socket)

        dictionary_of_sockets[path_to_socket] = permissions
        logger.info(f"The following socket was added to the dictionary: {path_to_socket}")

    return dictionary_of_sockets


def get_dictionary_of_actual_base_logs_and_permissions(install_dir="/opt/sophos-spl"):
    dictionary_of_logs = {}

    base_root_priv_log_files = glob.glob(f"{install_dir}/logs/base/*.log")
    base_low_priv_log_files = glob.glob(f"{install_dir}/logs/base/sophosspl/*.log")

    all_log_files = base_root_priv_log_files + base_low_priv_log_files

    for path_to_log_file in all_log_files:
        permissions = get_all_permissions(path_to_log_file)

        dictionary_of_logs[path_to_log_file] = permissions
        logger.info(f"The following socket was added to the dictionary: {path_to_log_file}")

    return dictionary_of_logs


def get_dictionary_of_actual_mcs_folders_and_permissions(install_dir="/opt/sophos-spl"):
    dictionary_of_mcs_folders = {}

    mcs_folders = [f"{install_dir}/base/mcs/action",
                   f"{install_dir}/base/mcs/certs",
                   f"{install_dir}/base/mcs/event",
                   f"{install_dir}/base/mcs/policy",
                   f"{install_dir}/base/mcs/response",
                   f"{install_dir}/base/mcs/status",
                   f"{install_dir}/base/mcs/tmp"
                   ]

    for mcs_folder in mcs_folders:
        permissions = get_all_permissions(mcs_folder)

        dictionary_of_mcs_folders[mcs_folder] = permissions
        logger.info(f"The following mcs folder was added to the dictionary: {mcs_folder}")

    return dictionary_of_mcs_folders


def get_directory_of_expected_mcs_folders_and_permissions(install_dir="/opt/sophos-spl"):
    return {
        f"{install_dir}/base/mcs/action": ["sophos-spl-local", "sophos-spl-group", "drwxr-x---"],
        f"{install_dir}/base/mcs/certs": ["root", "sophos-spl-group", "drwxr-x--x"],
        f"{install_dir}/base/mcs/event": ["root", "sophos-spl-group", "drwxrwx---"],
        f"{install_dir}/base/mcs/policy": ["sophos-spl-local", "sophos-spl-group", "drwxr-x---"],
        f"{install_dir}/base/mcs/response": ["root", "sophos-spl-group", "drwxrwx---"],
        f"{install_dir}/base/mcs/status": ["root", "sophos-spl-group", "drwxrwx---"],
        f"{install_dir}/base/mcs/tmp": ["sophos-spl-local", "sophos-spl-group", "drwx------"],
    }


def get_dictionary_of_expected_sockets_and_permissions(install_dir="/opt/sophos-spl"):
    return {
        f"{install_dir}/var/ipc/mcs_agent.ipc": ["sophos-spl-user", "sophos-spl-group", "srw-rw----"],
        f"{install_dir}/var/ipc/watchdog.ipc": ["root", "root", "srw-------"],
        f"{install_dir}/var/ipc/plugins/watchdogservice.ipc": ["root", "sophos-spl-group", "srw-rw----"],
        f"{install_dir}/var/ipc/plugins/tscheduler.ipc": ["sophos-spl-user", "sophos-spl-group", "srw-------"],
        f"{install_dir}/var/ipc/plugins/sdu.ipc": ["sophos-spl-user", "sophos-spl-group", "srw-------"],
        f"{install_dir}/var/ipc/plugins/updatescheduler.ipc": ["sophos-spl-updatescheduler", "sophos-spl-group",
                                                                "srw-rw----"]
    }


def get_dictionary_of_expected_base_logs_and_permissions(install_dir="/opt/sophos-spl"):
    return {
        f"{install_dir}/logs/base/watchdog.log": ["root", "root", "-rw-------"],
        f"{install_dir}/logs/base/wdctl.log": ["root", "root", "-rw-------"],
        f"{install_dir}/logs/base/sophosspl/mcs_envelope.log": ["sophos-spl-local", "sophos-spl-group", "-rw-------"],
        f"{install_dir}/logs/base/sophosspl/mcsrouter.log": ["sophos-spl-local", "sophos-spl-group", "-rw-------"],
        f"{install_dir}/logs/base/sophosspl/sophos_managementagent.log": ["sophos-spl-user", "sophos-spl-group",
                                                                           "-rw-------"],
        f"{install_dir}/logs/base/sophosspl/updatescheduler.log": ["sophos-spl-updatescheduler", "sophos-spl-group",
                                                                    "-rw-------"],
        f"{install_dir}/logs/base/sophosspl/tscheduler.log": ["sophos-spl-user", "sophos-spl-group", "-rw-------"],
        f"{install_dir}/logs/base/sophosspl/remote_diagnose.log": ["sophos-spl-user", "sophos-spl-group", "-rw-------"]
    }


def get_last_modified_time(path):
    assert os.path.exists(path), f"{path} does not exist"
    return os.stat(path).st_mtime  # epoch time


def find_most_recent_edit_time_from_list_of_files(list_of_files):
    most_recent_time = 0
    for file in list_of_files:
        cur_time = get_last_modified_time(file)
        if cur_time > most_recent_time:
            most_recent_time = cur_time
    return most_recent_time


def install_system_ca_cert(certificate_path):
    script = os.path.join(PathManager.get_utils_path(), "InstallCertificateToSystem.sh")
    os.chmod(script, 0o755)
    command = [script, certificate_path]
    logger.info(command)
    process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = process.communicate()
    logger.info(process.returncode)
    logger.info(out)
    logger.info(err)
    if process.returncode != 0:
        logger.info(f"stdout: {out}\nstderr: {err}")
        raise OSError(f"Failed to install \"{certificate_path}\" to system")


def install_system_ca_certs(certificate_paths):
    for path in certificate_paths:
        install_system_ca_cert(path)


def cleanup_system_ca_certs():
    utils_files_path = PathManager.get_utils_path()
    script = os.path.join(utils_files_path, "CleanupInstalledSystemCerts.sh")
    os.chmod(script, 0o755)
    logger.info(utils_files_path)
    logger.info(script)
    command = [script]
    process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = process.communicate()
    logger.info(process.returncode)
    if process.returncode != 0:
        logger.info(f"stdout: {out}\nstderr: {err}")
        raise OSError("Failed to cleanup system certs")


def turn_off_selinux():
    if os.path.isfile("/etc/sysconfig/selinux"):
        subprocess.check_call(["setenforce", "0"])


def turn_on_selinux():
    if os.path.isfile("/etc/sysconfig/selinux"):
        subprocess.check_call(["setenforce", "1"])


def get_interface_mac_addresses():
    # enum in psutil which can be used to find the tuples with mac addresses
    AF_LINK = 17
    dict_of_interfaces = psutil.net_if_addrs()
    mac_addresses = []
    for interface in iter(dict_of_interfaces.values()):
        for tuple in interface:
            if tuple.family == AF_LINK:
                if tuple.address != '00:00:00:00:00:00':
                    mac_addresses.append(tuple.address)
                    break
                break
    return mac_addresses


def get_dirname_of_path(path, require_exists=False):
    directory_path = os.path.dirname(path)
    if require_exists:
        assert os.path.exists(path), f"path \"{directory_path}\" does not exist"
    return directory_path


def get_basename_of_path(path):
    return os.path.basename(path)


def copy_file_if_exists(src, dest):
    if os.path.isfile(src):
        dest_dir = os.path.dirname(dest)
        if not os.path.isdir(dest_dir):
            os.makedirs(dest_dir)
        shutil.copyfile(src, dest)


def append_to_file_if_not_in_file(file_path, to_append):
    if not os.path.isfile(file_path):
        raise AssertionError(f"File path to append to does not exist: {file_path}")
    with open(file_path, 'w+') as f:
        contents = f.read()
        if to_append not in contents:
            f.write(f"\n{to_append}")



def generate_file(file_path, size_in_kib):
    size_in_kib = int(size_in_kib)
    #       12345678901234567890123456789012
    text = "GENERATED FILE CONTENTS01234567\n"
    assert len(text) == 32
    text = text * 32
    text = text.encode("UTF-8")
    assert len(text) == 1024
    with open(file_path, 'wb') as fout:
        for i in range(size_in_kib):
            fout.write(text)



def replace_service_with_sleep(service_name):
    dir_path = get_service_folder()

    file_path = os.path.join(dir_path, service_name)
    if not os.path.isfile(file_path):
        raise AssertionError(f"no service file detected at {file_path}")
    with open(file_path, 'r') as f:
        contents = f.readlines()
    new_contents = []
    for line in contents:
        if line.startswith("ExecStart"):
            new_contents.append("ExecStart=sleep 1000")
        else:
            new_contents.append(line)
    shutil.move(file_path, file_path + ".bak")
    with open(file_path, 'w') as f:
        f.write('\n'.join(new_contents))


def revert_service(service_name):
    dir_path = get_service_folder()

    file_path = os.path.join(dir_path, service_name)
    shutil.move(file_path + ".bak", file_path)


def get_service_folder():
    dir_path = ""
    if os.path.isdir("/lib/systemd/system"):
        dir_path = "/lib/systemd/system"
    elif os.path.isdir("/usr/lib/systemd/system"):
        dir_path = "/usr/lib/systemd/system"
    else:
        raise AssertionError("no service dir detected cannot replace service file")
    return dir_path


class ETCHostsWarehouseHandler:
    INTERNAL_WAREHOUSE_IP = '10.1.200.228'
    INTERNAL_WAREHOUSE_NAMES = ['dci.sophosupd.com', 'dci.sophosupd.net',
                                'd1.sophosupd.com', 'd1.sophosupd.net',
                                'd2.sophosupd.com', 'd2.sophosupd.net',
                                'd3.sophosupd.com', 'd3.sophosupd.net']

    def _get_lines_from_etc_hosts(self):
        with open('/etc/hosts', 'r') as etc_host_file:
            lines_of_etc_hosts = [line.strip() for line in etc_host_file]
            return lines_of_etc_hosts

    def _get_ip_internal_warehouse(self):
        return self.INTERNAL_WAREHOUSE_IP

    def _list_internal_warehouse(self):
        all_names = ' '.join(self.INTERNAL_WAREHOUSE_NAMES)
        return [f'{self.INTERNAL_WAREHOUSE_IP}\t{all_names}']

    def _get_etc_host_lines_filtered(self):
        lines_of_etc_hosts = self._get_lines_from_etc_hosts()
        warehouse_ip = self._get_ip_internal_warehouse()
        filtered = [entry for entry in lines_of_etc_hosts if warehouse_ip not in entry]
        return filtered

    def replace_etc_hosts(self, new_entries):
        content_of_etc_hosts = '\n'.join(new_entries)
        print(f"Content of the new etc/hosts: {content_of_etc_hosts}")
        with open('/etc/hosts.replace', 'w') as etc_host_file:
            etc_host_file.write(content_of_etc_hosts)
        if not os.path.exists('/etc/hosts.bkp'):
            shutil.move('/etc/hosts', '/etc/hosts.bkp')
        shutil.move('/etc/hosts.replace', '/etc/hosts')

    def restore_from_backup(self):
        if not os.path.exists('/etc/hosts.bkp'):
            raise AssertionError("/etc/hosts.bkp is not present")
        shutil.move('/etc/hosts.bkp', '/etc/hosts')

    def setup_internal_warehouse(self):
        filtered = self._get_etc_host_lines_filtered()
        final_etc_hosts = filtered + [''] + self._list_internal_warehouse() + ['']
        self.replace_etc_hosts(final_etc_hosts)

    def clear_internal_warehouse(self):
        filtered = self._get_etc_host_lines_filtered() + ['']
        self.replace_etc_hosts(filtered)


def time_since_epoch_hours():
    return round(time.time() / 3600, 1)


def setup_etc_hosts_to_connect_to_internal_warehouse():
    etc = ETCHostsWarehouseHandler()
    etc.setup_internal_warehouse()


def clear_etc_hosts_of_entries_to_connect_to_internal_warehouse():
    etc = ETCHostsWarehouseHandler()
    etc.clear_internal_warehouse()

def running_processes_should_match_the_count(comm_name, match_dir_path, count):
    count = int(count)
    match_procs = [p for p in psutil.process_iter() if comm_name in p.name() and match_dir_path in p.exe()]
    print(f"Matched processes: {match_procs}")
    if len(match_procs) != count:
        raise AssertionError(f"Expected {count} processes. Found: {len(match_procs)}. Information: {match_procs}")


def is_ubuntu():
    return platform.dist()[0] == "Ubuntu"


def get_uid_from_username(username):
    return pwd.getpwnam(username).pw_uid


def get_gid_from_groupname(group_name: str):
    return grp.getgrnam(group_name).gr_gid


def get_gcc_version_from_lib(lib_path) -> str:
    result = subprocess.run(["strings", lib_path], stdout=subprocess.PIPE)
    output = result.stdout.decode('utf-8')
    lines = output.split("\n")
    for line in lines:
        if "GCC: (GNU)" in line:
            return line
    return "unknown"


def check_libs_for_consistent_gcc_version(directory):
    libs = []
    for lib in os.listdir(directory):
        if lib.endswith(".so"):
            if lib in ["libstdc++.so", "libgcc_s.so", ""]:
                logger.info(f"Skipping: {lib}")
                continue
            lib_path = os.path.join(directory, lib)
            lib_info = {"name": lib, "path": lib_path, "gcc_version": get_gcc_version_from_lib(lib_path)}
            libs.append(lib_info)
    gcc_versions = []
    for lib in libs:
        if lib['gcc_version'] not in gcc_versions:
            gcc_versions.append(lib['gcc_version'])
        logger.info(f"{lib['name']} - {lib['gcc_version']}")
    logger.info(f"All versions detected: {gcc_versions}")
    if len(gcc_versions) != 1:
        raise AssertionError("Multiple GCC versions found across product libs")


def does_file_exist(path) -> bool:
    return bool(os.path.exists(path))


def does_file_not_exist(path) -> bool:
    return not does_file_exist(path)


def does_file_contain_word(path, word) -> bool:
    with open(path) as f:
        pattern = re.compile(r'\b({0})\b'.format(word), flags=re.IGNORECASE)
        return bool(pattern.search(f.read()))


def does_file_not_contain(path, word) -> bool:
    return not does_file_contain_word(path, word)

def journalctl_contains(text) -> bool:
    out = subprocess.check_output(['journalctl', '--no-pager', '--since', '5 minutes ago'],
                          stdin=open('/dev/null', 'wb'))
    output = out.decode('utf-8')
    return text in output

def get_hostname() -> str:
    return socket.gethostname()

def directory_exists(path) -> bool:
    return os.path.isdir(path)

def machine_architecture() -> str:
    architecture = os.uname().machine
    if architecture == "x86_64":
        return "LINUX_INTEL_LIBC6"
    elif architecture == "aarch64":
        return "LINUX_ARM64"
    return ""


def dump_df():
    df = subprocess.run(["df", "-hl"], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    logger.info(f"df -hl {df.returncode}: {df.stdout}")
