#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2019-2020 Sophos Plc, Oxford, England.
# All rights reserved.


import os
import stat
import glob
import grp
import pwd
import signal
import time
import subprocess
from robot.api import logger
import shutil
import tarfile
import psutil
import platform

import PathManager


def ensure_chmod_file_matches(filepath, expected_chmod_string):
    given_cmod = oct(os.stat(filepath)[stat.ST_MODE])[-3:]
    if given_cmod != expected_chmod_string:
        raise AssertionError( "Filepath {} chmod is {}. Expected {}".format(filepath, given_cmod, expected_chmod_string))


def ensure_owner_and_group_matches(filepath, username, groupname):
    stat_info = os.stat(filepath)
    user = pwd.getpwuid(stat_info.st_uid)[0]
    group = grp.getgrgid(stat_info.st_gid)[0]
    if username != user:
        raise AssertionError("Filepath {} owner is {}. Expected {}.".format(filepath, user, username))
    if group != groupname:
        raise AssertionError("Filepath {} belongs to group {}. Expected {}", filepath, group, groupname)

def pids_of_file(file_path):

    if not os.path.exists(file_path):
        raise AssertionError("Not a valid path: {}".format(file_path))
    pids_of = []

    try:
        output = subprocess.check_output(['pidof', file_path])
        output = output.decode()
        if len(output):
            pids_of = [int(p) for p in output[:-1].split(' ')]
            logger.info("Pids found: {}".format(pids_of))
    except subprocess.CalledProcessError as ex: 
        # this is expected, as the process was executed but could 
        # not find any running process for the path
        # hence no process to kill
        logger.debug(repr(ex)) # for debug purpose only
        pass

    except Exception as ex: 
        # any other exception is not expected
        raise
    return pids_of

def check_if_process_has_osquery_netlink(file_path):
    pids = pids_of_file(file_path)

    if len(pids) < 2:
        raise AssertionError("not all osquery processes are not running only {} processes running".format(len(pids)))
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

def check_process_running( file_path, expect='Running'):
    """
    file_path path of the file
    expect= Running or NotRunning    
    """
    if expect not in ['Running', 'NotRunning']:
        raise AssertionError('Expect option not supported: {}'.format(expect))
    pids_of = pids_of_file(file_path)
    if pids_of and expect=='Running':
        return []
    if pids_of and expect=='NotRunning':
        raise AssertionError("Process still running {} with the pids: {}".format(file_path, pids_of))
    if not pids_of and expect=='NotRunning':
        return []
    if not pids_of and expect=='Running':
        raise AssertionError("No Process running with the path: {}".format(file_path))
    return pids_of

def kill_by_file(file_path):
    pids_of = pids_of_file(file_path)
    if pids_of:
        
        for pid in pids_of: 
            logger.info("Issuing kill command to pid: {}".format(pid))
            os.kill(int(pid), signal.SIGTERM) 
    else: 
        logger.info("No process to kill")
    return pids_of

def kill_by_command(command):
    logger.info("Attempt to kill process by command: {}".format(command))
    if os.path.exists(command):
        kill_by_file(command)
    try:
        # remove the new line at the end
        file_path = subprocess.check_output(['which',command])[:-1]
        
        logger.info('Command path: {}'.format(file_path))
    except Exception as ex:
        logger.info(repr(ex))
        raise AssertionError( "Command path not found: {}".format(command))
    kill_by_file(file_path)


def kill_by_file_and_wait_it_to_shutdown(file_path, secs2wait, number_of_attempts):
    pids_before_kill = set(kill_by_file(file_path))
    logger.info("Sent the signal kill to processes: {}".format(pids_before_kill))
    number_of_attempts = int(number_of_attempts)
    secs2wait = int(secs2wait)
    attempt=0
    while attempt < number_of_attempts: 
        attempt += 1
        time.sleep(secs2wait)
        pids_after_kill = set(pids_of_file(file_path))
        logger.info("Current processes running: {}".format(pids_after_kill))
        if len(pids_before_kill.intersection(pids_after_kill)) == 0: 
            return

    raise AssertionError("Process not killed after {} seconds".format(secs2wait * number_of_attempts))


def remove_dir_if_exists(dir):
    if os.path.isdir(dir):
        logger.info("Removing dir: ".format(dir))
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

def check_files_have_not_been_removed(install_directory, removed_file_list, root_directory_to_check, files_to_ignore_in_check):
    # function used to check that files have not been removed by the installation cleanup processes even if
    # the files are listed in the remove_files_list created by the manifest diff
    # when the files listed are outside of the realm of the component file set.

    #install_directory: location of sophos-spl
    #removed_file_list: file path of the file created by the manifest diff named removedFiles_manifest files in this list should be prevented from being deleted
    #root_directory_to_check: component root direcotry
    #files_to_ignore_in_check: file path to the files to delete dat file.  These files will be correctly deteted by the correct component
    #   For example if test is checking plugin does not delete base files, the filestodelete.dat file should be the one for base.

    if os.path.isfile(removed_file_list):

        path_to_remove = "files/"

        with open(removed_file_list, "r") as removed_input_file:
            for removed_input_line in removed_input_file:
                removed_input_line =  removed_input_line.strip()
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
                                raise AssertionError("A file has been removed that should have been prevented from being deleted: {}".format(full_path))




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


def get_file_owner(filepath):
    owner = subprocess.check_output(['stat', '-c', '%U', filepath]).decode().strip()
    return owner


def get_file_group(filepath):
    group = subprocess.check_output(['stat', '-c', '%G', filepath]).decode().strip()
    return group


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
    assert actual_owner == expected_owner, "expected owner: {}, doesn't match actual: {}". \
        format(expected_owner, actual_owner)


def check_group_matches(expected_group, filepath):
    actual_group = get_file_group(filepath)
    assert actual_group == expected_group, "expected group: {}, doesn't match actual: {}". \
        format(expected_group, actual_group)


def check_file_permissions_match(expected_perms, filepath):
    actual_perms = get_file_permissions(filepath)
    assert actual_perms == expected_perms, "expected file permissions: {}, don't match actual: {}". \
        format(expected_perms, actual_perms)


def file_exists_with_permissions(path_to_file, owner, group, permissions):
    assert os.path.isfile(path_to_file), "{} is not a file".format(path_to_file)
    check_owner_matches(owner, path_to_file)
    check_group_matches(group, path_to_file)
    check_file_permissions_match(permissions, path_to_file)


def socket_exists_with_permissions(path_to_socket, owner, group, permissions):
    mode = os.stat(path_to_socket).st_mode
    assert stat.S_ISSOCK(mode), "{} is not a socket".format(path_to_socket)
    check_owner_matches(owner, path_to_socket)
    check_group_matches(group, path_to_socket)
    check_file_permissions_match(permissions, path_to_socket)


def get_dictionary_of_actual_sockets_and_permissions():
    dictionary_of_sockets = {}

    base_ipc_files = glob.glob('/opt/sophos-spl/var/ipc/*.ipc')
    plugin_ipc_files = glob.glob('/opt/sophos-spl/var/ipc/plugins/*.ipc')

    all_ipc_files = base_ipc_files + plugin_ipc_files

    for path_to_socket in all_ipc_files:
        mode = os.stat(path_to_socket).st_mode
        assert stat.S_ISSOCK(mode), "{} is not a socket".format(path_to_socket)

        permissions = get_all_permissions(path_to_socket)

        dictionary_of_sockets[path_to_socket] = permissions
        logger.info("The following socket was added to the dictionary: {}".format(path_to_socket))

    return dictionary_of_sockets


def get_dictionary_of_actual_base_logs_and_permissions():
    dictionary_of_logs = {}

    base_root_priv_log_files = glob.glob('/opt/sophos-spl/logs/base/*.log')
    base_low_priv_log_files = glob.glob('/opt/sophos-spl/logs/base/sophosspl/*.log')

    all_log_files = base_root_priv_log_files + base_low_priv_log_files

    for path_to_log_file in all_log_files:
        permissions = get_all_permissions(path_to_log_file)

        dictionary_of_logs[path_to_log_file] = permissions
        logger.info("The following socket was added to the dictionary: {}".format(path_to_log_file))

    return dictionary_of_logs

def get_dictionary_of_actual_mcs_folders_and_permissions():
    dictionary_of_mcs_folders = {}

    mcs_folders = ["/opt/sophos-spl/base/mcs/action",
                   "/opt/sophos-spl/base/mcs/certs",
                   "/opt/sophos-spl/base/mcs/event",
                   "/opt/sophos-spl/base/mcs/policy",
                   "/opt/sophos-spl/base/mcs/response",
                   "/opt/sophos-spl/base/mcs/status",
                   "/opt/sophos-spl/base/mcs/tmp"
                   ]

    for mcs_folder in mcs_folders:
        permissions = get_all_permissions(mcs_folder)

        dictionary_of_mcs_folders[mcs_folder] = permissions
        logger.info("The following mcs folder was added to the dictionary: {}".format(mcs_folder))

    return dictionary_of_mcs_folders

def get_directory_of_expected_mcs_folders_and_permissions():
    return {
        "/opt/sophos-spl/base/mcs/action":      ["sophos-spl-local", "sophos-spl-group", "drwxr-x---"],
        "/opt/sophos-spl/base/mcs/certs":       ["root", "sophos-spl-group", "drwxr-x---"],
        "/opt/sophos-spl/base/mcs/event":       ["sophos-spl-local", "sophos-spl-group", "drwxrwx---"],
        "/opt/sophos-spl/base/mcs/policy":      ["sophos-spl-local", "sophos-spl-group", "drwxr-x---"],
        "/opt/sophos-spl/base/mcs/response":    ["sophos-spl-local", "sophos-spl-group", "drwxr-x---"],
        "/opt/sophos-spl/base/mcs/status":      ["sophos-spl-local", "sophos-spl-group", "drwxrwx---"],
        "/opt/sophos-spl/base/mcs/tmp":         ["sophos-spl-local", "sophos-spl-group", "drwxr-x---"],
    }

def get_dictionary_of_expected_sockets_and_permissions():
    return {
        "/opt/sophos-spl/var/ipc/mcs_agent.ipc":                ["sophos-spl-user", "sophos-spl-group", "srw-------"],
        "/opt/sophos-spl/var/ipc/publisherdatachannel.ipc":     ["sophos-spl-user", "sophos-spl-group", "srw-------"],
        "/opt/sophos-spl/var/ipc/subscriberdatachannel.ipc":    ["sophos-spl-user", "sophos-spl-group", "srw-------"],
        "/opt/sophos-spl/var/ipc/watchdog.ipc":                 ["root",            "root",             "srw-------"],
        "/opt/sophos-spl/var/ipc/plugins/watchdogservice.ipc":  ["root", "sophos-spl-group", "srw-rw----"],
        "/opt/sophos-spl/var/ipc/plugins/tscheduler.ipc":       ["sophos-spl-user", "sophos-spl-group", "srw-------"],
        "/opt/sophos-spl/var/ipc/plugins/updatescheduler.ipc":  ["sophos-spl-user", "sophos-spl-group", "srwx------"]
    }


def get_dictionary_of_expected_base_logs_and_permissions():
    return {
        "/opt/sophos-spl/logs/base/watchdog.log":                         ["root", "root", "-rw-------"],
        "/opt/sophos-spl/logs/base/wdctl.log":                            ["root", "root", "-rw-------"],
        "/opt/sophos-spl/logs/base/comms_component.log":                  ["root", "root", "-rw-------"],
        "/opt/sophos-spl/logs/base/sophosspl/mcs_envelope.log":           ["sophos-spl-user", "sophos-spl-group", "-rw-------"],
        "/opt/sophos-spl/logs/base/sophosspl/mcsrouter.log":              ["sophos-spl-user", "sophos-spl-group", "-rw-------"],
        "/opt/sophos-spl/logs/base/sophosspl/sophos_managementagent.log": ["sophos-spl-user", "sophos-spl-group", "-rw-------"],
        "/opt/sophos-spl/logs/base/sophosspl/updatescheduler.log":        ["sophos-spl-user", "sophos-spl-group", "-rw-------"],
        "/opt/sophos-spl/logs/base/sophosspl/tscheduler.log":             ["sophos-spl-user", "sophos-spl-group", "-rw-------"]
    }


def get_last_modified_time(path):
    assert os.path.exists(path), "{} does not exist".format(path)
    return os.stat(path).st_mtime # epoch time


def find_most_recent_edit_time_from_list_of_files(list_of_files):
    most_recent_time = 0
    for file in list_of_files:
        cur_time = get_last_modified_time(file)
        if cur_time > most_recent_time:
            most_recent_time = cur_time
    return most_recent_time

def install_system_ca_cert(certificate_path):
    script = os.path.join(PathManager.get_support_file_path(), "InstallCertificateToSystem.sh")
    command = [script, certificate_path]
    logger.info(command)
    process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = process.communicate()
    logger.info(process.returncode)
    logger.info(out)
    logger.info(err)
    if process.returncode != 0:
        logger.info("stdout: {}\nstderr: {}".format(out, err))
        raise OSError("Failed to install \"{}\" to system".format(certificate_path))


def install_system_ca_certs(certificate_paths):
    for path in certificate_paths:
        install_system_ca_cert(path)


def cleanup_system_ca_certs():
    support_files_path = PathManager.get_support_file_path()
    script = os.path.join(support_files_path, "CleanupInstalledSystemCerts.sh")
    logger.info(support_files_path)
    logger.info(script)
    command = [script]
    process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = process.communicate()
    logger.info(process.returncode)
    if process.returncode != 0:
        logger.info("stdout: {}\nstderr: {}".format(out, err))
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
        assert os.path.exists(path), "path \"{}\" does not exist".format(directory_path)
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
        raise AssertionError("File path to append to does not exist: {}".format(file_path))
    with open(file_path, 'w+') as f:
        contents = f.read()
        if to_append not in contents:
            f.write("\n{}".format(to_append))

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
        return ['{}\t{}'.format(self.INTERNAL_WAREHOUSE_IP, all_names)]

    def _get_etc_host_lines_filtered(self):
        lines_of_etc_hosts = self._get_lines_from_etc_hosts()
        warehouse_ip = self._get_ip_internal_warehouse()
        filtered = [entry for entry in lines_of_etc_hosts if warehouse_ip not in entry]
        return filtered

    def replace_etc_hosts(self, new_entries):
        content_of_etc_hosts = '\n'.join(new_entries)
        print("Content of the new etc/hosts: {}".format(content_of_etc_hosts))
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



def setup_etc_hosts_to_connect_to_internal_warehouse():
    etc = ETCHostsWarehouseHandler()
    etc.setup_internal_warehouse()


def clear_etc_hosts_of_entries_to_connect_to_internal_warehouse():
    etc = ETCHostsWarehouseHandler()
    etc.clear_internal_warehouse()


def install_internal_warehouse_certs():
    directory_path =   '/mnt/filer6/linux/SSPL/tools/setup_sspl/certs/internal_certs/'
    certs = []
    for file_name in os.listdir(directory_path):
        if file_name.endswith('crt'):
            certs.append(os.path.join(directory_path, file_name))
    install_system_ca_certs(certs)


def running_processes_should_match_the_count(comm_name, match_dir_path, count):
    count = int(count)
    match_procs = [p for p in psutil.process_iter() if comm_name in p.name() and match_dir_path in p.exe()]
    print("Matched processes: {}".format(match_procs))
    if len(match_procs) != count:
        raise AssertionError("Expected {} processes. Found: {}. Information: {}".format(count, len(match_procs), match_procs))


def is_ubuntu():
    return platform.dist()[0] == "Ubuntu"

