#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2018-2020 Sophos Plc, Oxford, England.
# All rights reserved.

import os
import shutil
import subprocess
import grp
import pwd
import tempfile
import glob
import json
import stat
import xml.dom.minidom
import re
import configparser
import time

import PathManager

from robot.api import logger
from robot.libraries.BuiltIn import BuiltIn
import robot.libraries.BuiltIn

PUB_SUB_LOGGING_DIST_LOCATION = "/tmp/pub_sub_logging_dist"


def get_variable(varName, defaultValue=None):
    try:
        return BuiltIn().get_variable_value("${%s}" % varName)
    except robot.libraries.BuiltIn.RobotNotRunningError:
        return os.environ.get(varName, defaultValue)

def get_sophos_install():
    return get_variable("SOPHOS_INSTALL")


def get_full_installer():
    OUTPUT = os.environ.get("OUTPUT")
    BASE_DIST = os.environ.get("BASE_DIST")

    logger.debug("Getting installer BASE_DIST: {}, OUTPUT: {}".format(BASE_DIST, OUTPUT))

    if BASE_DIST is not None:
        installer = os.path.join(BASE_DIST, "install.sh")
        if os.path.isfile(installer):
            logger.debug("Using installer from BASE_DIST: {}".format(installer))
            return installer

    if OUTPUT is not None:
        installer = os.path.join(OUTPUT, "SDDS-COMPONENT", "install.sh")
        if os.path.isfile(installer):
            logger.debug("Using installer from OUTPUT: {}".format(installer))
            return installer

    installer = os.path.join("../everest-base/build64/distribution/install.sh")
    if os.path.isfile(installer):
        logger.debug("Using installer from build64: {}".format(installer))
        return installer

    installer = os.path.join("../everest-base/cmake-build-debug/distribution/install.sh")
    if os.path.isfile(installer):
        logger.debug("Using installer from cmake-build-debug: {}".format(installer))
        return installer

    raise Exception("Failed to find installer")


def get_folder_with_installer():
    return os.path.dirname(get_full_installer())

def get_plugin_sdds(plugin_name, environment_variable_name, candidates=None):
    # Check if an environment variable has been set pointing to the example plugin sdds
    SDDS_PATH = os.environ.get(environment_variable_name)

    env_path = ""
    if SDDS_PATH is not None:
        env_path = SDDS_PATH
        if os.path.exists(SDDS_PATH):
            logger.info("Using plugin sdds defined by {}: {}".format(environment_variable_name, SDDS_PATH))
            return SDDS_PATH

    fullpath_candidates = [os.path.abspath(p) for p in candidates]

    for plugin_sdds in fullpath_candidates:
        if os.path.exists(plugin_sdds):
            logger.info("Using local plugin {} sdds: {}".format(plugin_name, plugin_sdds))
            return plugin_sdds

    raise Exception("Failed to find plugin {} sdds. Attempted: ".format(plugin_name) + ' '.join(fullpath_candidates) + " " + env_path)


class MDREntry(object):
    def __init__(self, rigid_name, sdds):
        self.rigid_name = rigid_name
        self.sdds = sdds

def get_component_suite_sdds_entry(name, environment_variable_name, candidates, use_env_overrides=True):
    expected_directory_name = environment_variable_name.replace("_","-")
    entry_candidates = [os.path.join(candidate, expected_directory_name) for candidate in candidates]
    if not use_env_overrides:
        environment_variable_name = None
    sdds = get_plugin_sdds(name, environment_variable_name, entry_candidates)
    return MDREntry(name, sdds)



def get_exampleplugin_sdds():
    candidates = []
    local_path_to_plugin = PathManager.find_local_component_dir_path("exampleplugin")
    if local_path_to_plugin:
        candidates.append(os.path.join(local_path_to_plugin, "build64/sdds"))
        candidates.append(os.path.join(local_path_to_plugin, "cmake-build-debug/sdds"))
    return get_plugin_sdds("Example Plugin", "EXAMPLEPLUGIN_SDDS", candidates)


def get_event_processor_plugin_sdds():
    candidates = []
    local_path_to_plugin = PathManager.find_local_component_dir_path("sspl-plugin-eventprocessor")
    if local_path_to_plugin:
        candidates.append(os.path.join(local_path_to_plugin, "build64/sdds"))
        candidates.append(os.path.join(local_path_to_plugin, "cmake-build-debug/sdds"))
    return get_plugin_sdds("Event Processor Plugin", "SSPL_PLUGIN_EVENTPROCESSOR_SDDS", candidates)


def get_sspl_audit_plugin_sdds():
    candidates = []
    local_path_to_plugin = PathManager.find_local_component_dir_path("sspl-plugin-eventprocessor")
    if local_path_to_plugin:
        candidates.append(os.path.join(local_path_to_plugin, "build64/sdds"))
        candidates.append(os.path.join(local_path_to_plugin, "cmake-build-debug/sdds"))
    return get_plugin_sdds("SSPL Audit Plugin", "SSPL_AUDIT_PLUGIN_SDDS", candidates)



class MDRSuite:
    def __init__(self, dbos, osquery, mdr_plugin, mdr_suite):
        self.dbos = dbos
        self.osquery = osquery
        self.mdr_plugin = mdr_plugin
        self.mdr_suite = mdr_suite

def get_sspl_mdr_component_suite():
    candidates = []
    local_path_to_plugin = PathManager.find_local_component_dir_path("sspl-plugin-mdr-componentsuite")
    if local_path_to_plugin:
        candidates.append(os.path.join(local_path_to_plugin, "output"))
    dbos = get_component_suite_sdds_entry("ServerProtectionLinux-MDR-DBOS-Component",  "SDDS_SSPL_DBOS_COMPONENT", candidates)
    osquery = get_component_suite_sdds_entry("SDDS-SSPL-OSQUERY-COMPONENT",  "SDDS_SSPL_OSQUERY_COMPONENT", candidates)
    mdr_plugin = get_component_suite_sdds_entry("ServerProtectionLinux-MDR-Control",  "SDDS_SSPL_MDR_COMPONENT", candidates)
    mdr_suite = get_component_suite_sdds_entry("ServerProtectionLinux-Plugin-MDR",  "SDDS_SSPL_MDR_COMPONENT_SUITE", candidates)
    return MDRSuite(dbos, osquery, mdr_plugin, mdr_suite)

def get_sspl_mdr_component_suite_1():
    release_1_override = os.environ.get("SDDS_SSPL_MDR_COMPONENT_SUITE_RELEASE_1_0", None)

    if release_1_override is None:
        candidates = ["/uk-filer5/prodro/bir/sspl-mdr-componentsuite/1-0-0-2/217070/output"]
    else:
        candidates = [release_1_override]
    dbos = get_component_suite_sdds_entry("ServerProtectionLinux-MDR-DBOS-Component",  "SDDS_SSPL_DBOS_COMPONENT", candidates, use_env_overrides=False)
    osquery = get_component_suite_sdds_entry("SDDS-SSPL-OSQUERY-COMPONENT",  "SDDS_SSPL_OSQUERY_COMPONENT", candidates, use_env_overrides=False)
    mdr_plugin = get_component_suite_sdds_entry("ServerProtectionLinux-MDR-Control",  "SDDS_SSPL_MDR_COMPONENT", candidates, use_env_overrides=False)
    mdr_suite = get_component_suite_sdds_entry("ServerProtectionLinux-Plugin-MDR",  "SDDS_SSPL_MDR_COMPONENT_SUITE", candidates, use_env_overrides=False)
    return MDRSuite(dbos, osquery, mdr_plugin, mdr_suite)

def get_sspl_mdr_plugin_sdds():
    candidates = []
    local_path_to_plugin = PathManager.find_local_component_dir_path("sspl-plugin-mdr-component")
    if local_path_to_plugin:
        candidates.append(os.path.join(local_path_to_plugin, "build64/sdds"))
        candidates.append(os.path.join(local_path_to_plugin, "cmake-build-debug/sdds"))
    return get_plugin_sdds("SSPL MDR Plugin", "SSPL_MDR_PLUGIN_SDDS", candidates)

def get_sspl_live_response_plugin_sdds():
    candidates = []
    local_path_to_plugin = PathManager.find_local_component_dir_path("liveterminal")
    if local_path_to_plugin:
        candidates.append(os.path.join(local_path_to_plugin, "build64/sdds"))
        candidates.append(os.path.join(local_path_to_plugin, "cmake-build-debug/sdds"))
    return get_plugin_sdds("SSPL LiveResponse Plugin", "SSPL_LIVERESPONSE_PLUGIN_SDDS", candidates)

def get_sspl_edr_plugin_sdds():
    candidates = []
    local_path_to_plugin = PathManager.find_local_component_dir_path("sspl-plugin-edr-component")
    if local_path_to_plugin:
        candidates.append(os.path.join(local_path_to_plugin, "build64/sdds"))
        candidates.append(os.path.join(local_path_to_plugin, "cmake-build-debug/sdds"))
    return get_plugin_sdds("SSPL EDR Plugin", "SSPL_EDR_PLUGIN_SDDS", candidates)

def get_sspl_base_sdds_version_0_5():
    candidates = ["/uk-filer5/prodro/bir/sspl-base/0-5-0-223/213552/output/SDDS-COMPONENT/"]
    return get_plugin_sdds("SSPL Base Version 0.5", "SSPL_BASE_SDDS_RELEASE_0_5", candidates)

def get_sspl_base_sdds_version_1():  
    candidates = ["/uk-filer5/prodro/bir/sspl-base/1-0-0-74/216976/output/SDDS-COMPONENT/"]
    return get_plugin_sdds("SSPL Base Version 1.0.0", "SSPL_BASE_SDDS_RELEASE_1_0", candidates)

def get_telemetry_supplement_version_1_path():
    telemetry_supplement_release_1_override = os.environ.get("RELEASE_1_TELEMETRY_SUPPLEMENT", None)

    if telemetry_supplement_release_1_override is None:
        return "/uk-filer5/prodro/bir/sspl-telemetry-supplement/1-0-0-3/217066/output/SDDS-SSPL-TELEMSUPP-COMPONENT/telemetry-config.json"
    else:
        return telemetry_supplement_release_1_override


def get_sspl_example_plugin_sdds_version_0_5():
    candidates = ["/uk-filer5/prodro/bir/sspl-exampleplugin/0-5-0-43/213556/output/SDDS-COMPONENT/"]
    return get_plugin_sdds("SSPL Example Plugin Version 0.5", "SSPL_EXAMPLE_PLUGIN_SDDS_RELEASE_0_5", candidates)


def copy_exampleplugin_sdds():
    original_location = get_exampleplugin_sdds()
    tmpdir = os.path.join(tempfile.mkdtemp(prefix="robot-example-plugin"), "sdds")
    shutil.copytree(original_location, tmpdir)
    return tmpdir


def copy_event_processor_plugin_sdds():
    original_location = get_event_processor_plugin_sdds()
    tmpdir = os.path.join(tempfile.mkdtemp(prefix="robot-event-processsor-plugin"), "sdds")
    shutil.copytree(original_location, tmpdir)
    return tmpdir


def copy_sspl_audit_plugin_sdds():
    original_location = get_sspl_audit_plugin_sdds()
    tmpdir = os.path.join(tempfile.mkdtemp(prefix="robot-audit-plugin"), "sdds")
    shutil.copytree(original_location, tmpdir)
    return tmpdir


def _copy_suite_entry_to(root_target_directory, mdr_entry):
    directory_name = mdr_entry.rigid_name
    src_directory = mdr_entry.sdds
    _, src_name = os.path.split(src_directory)

    target_directory = os.path.join(root_target_directory, directory_name)

    logger.info("Copying sdds:{} to target_directory: {}".format(src_directory, root_target_directory))
    logger.info("Copying sdds:{} to target_directory: {}".format(src_directory, target_directory))

    if os.path.exists(target_directory):
        shutil.rmtree(target_directory)
    try:
        shutil.copytree(src_directory, target_directory)
    except Exception as ex:

        logger.info(str(ex))
        list_entries = ','.join(os.listdir(root_target_directory))
        logger.info("Entries in the target directory: {}".format(list_entries))


def copy_mdr_component_suite_to(target_directory, mdr_component_suite):
    _copy_suite_entry_to(target_directory, mdr_component_suite.dbos)
    _copy_suite_entry_to(target_directory, mdr_component_suite.osquery)
    _copy_suite_entry_to(target_directory, mdr_component_suite.mdr_plugin)
    _copy_suite_entry_to(target_directory, mdr_component_suite.mdr_suite)
    list_entries = ','.join(os.listdir(target_directory))
    logger.info("Entries in the target directory: {}".format(list_entries))


def get_xml_node_text(node):
    return "".join(t.nodeValue for t in node.childNodes if t.nodeType == t.TEXT_NODE)


def check_sdds_import_matches_rigid_name(expected_rigidname, source_directory):
    sddsimportpath = os.path.join(source_directory, "SDDS-Import.xml")
    data = open(sddsimportpath).read()
    dom = xml.dom.minidom.parseString(data)
    try:
        rigidnames = dom.getElementsByTagName("RigidName")
        assert len(rigidnames) == 1
        actual_rigidname = get_xml_node_text(rigidnames[0])
        if expected_rigidname != actual_rigidname:
            raise AssertionError("Rigidname wrong. Expected {} but actual is: {}".format(expected_rigidname, actual_rigidname))
    finally:
        dom.unlink()


def run_full_installer_expecting_code(expected_code, *args):
    if get_variable("PUB_SUB_LOGGING"):
        installer = os.path.join(PUB_SUB_LOGGING_DIST_LOCATION, "install.sh")
    else:
        installer = get_full_installer()
    logger.info("Installer path: " + str(installer))
    return run_full_installer_from_location_expecting_code(installer, expected_code, *args)


def run_full_installer_from_location_expecting_code(install_script_location, expected_code, *args):
    # Robot passes in strings.
    expected_code = int(expected_code)

    arg_list = ["bash", "-x", install_script_location]
    arg_list += list(args)
    logger.debug("Env Variables: {}".format(os.environ))
    logger.info("Run installer: {}".format(arg_list))
    pop = subprocess.Popen(arg_list, env=os.environ, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    output, all_steps = pop.communicate()
    actual_code = pop.returncode
    logger.info(output)
    logger.debug(all_steps)

    if actual_code != expected_code:
        logger.error(output)
        logger.error(all_steps)
        raise AssertionError("Installer exited with {} rather than {}".format(actual_code, expected_code))

    return output+all_steps

def filter_trace_from_installer_output(console_output):
    lines = console_output.split("\n")
    filtered_lines = []
    for line in lines:
        if len(line) > 0 and line[0] != "+":
            filtered_lines.append(line)

    return "\n".join(filtered_lines)

def validate_incorrect_glibc_console_output(console_output):
    filtered_console_output = filter_trace_from_installer_output(console_output.decode('utf-8'))

    # This regex may seem like slight overkill for the test.
    # The logic in the installer relies on the glibc version being of the form [0-9]*\.[0-9]
    # So this check simultaneously checks the presence of an error message, and will alert us if something funky
    # has happened with the glibc version format
    regex_pattern = \
        r"^Failed to install on unsupported system\. Detected GLIBC version ([0-9]*\.[0-9]*) < required ([0-9]*\.[0-9]*)$"
    for line in filtered_console_output.split("\n"):
        match_object = re.match(regex_pattern, line)
        if match_object:
            logger.info(match_object.group(0))
            logger.info(match_object.group(1))
            logger.info(match_object.group(2))
            return
    raise AssertionError("output:\n\t{}\ndoes not contain a line matching patter:\n\t{}".format(
        filtered_console_output,
        regex_pattern))

def get_glibc_version_from_full_installer():
    installer = get_full_installer()
    build_variable_setting_line="BUILD_LIBC_VERSION="
    regex_pattern = r"{}([0-9]*\.[0-9]*)".format(build_variable_setting_line)
    with open(installer, "r") as installer_file:
        for line in installer_file.readlines():
            if build_variable_setting_line in line:
                match_object = re.match(regex_pattern, line)
                version = match_object.group(1)
                return version
    raise AssertionError("Installer: {}\nDid not contain line matching: {}".format(installer, regex_pattern))

def add_environment_variables_to_installer_plugin_file(distDir, fileName, keyValuePairDict):
    targetJsonFile = os.path.join(distDir, "installer", "plugins", fileName)
    env_dict = {"environmentVariables": []}
    for key, value in keyValuePairDict.items():
        env_dict["environmentVariables"].append({"name": key, "value": value})

    with open(targetJsonFile) as f:
        data = json.load(f)
    data.update(env_dict)
    with open(targetJsonFile, 'w+') as f:
        json.dump(data, f)


def create_full_installer_with_pub_sub_logging():
    installerDir = os.path.dirname(get_full_installer())
    print("Installer path copied for pub sub logging addition: " + str(installerDir))
    if os.path.isdir(PUB_SUB_LOGGING_DIST_LOCATION):
        logger.info("{} already exists so will delete it".format(PUB_SUB_LOGGING_DIST_LOCATION))
        clean_up_full_installer_with_pub_sub_logging()

    shutil.copytree(installerDir, PUB_SUB_LOGGING_DIST_LOCATION)

    environmentDict = {"SOPHOS_PUB_SUB_ROUTER_LOGGING": "1"}
    targetFile = "managementagent.json"
    logger.info("Altering management agent file: {}  to add the following environment variables: {}.".format(targetFile, environmentDict))
    add_environment_variables_to_installer_plugin_file(PUB_SUB_LOGGING_DIST_LOCATION, targetFile, environmentDict)


def clean_up_full_installer_with_pub_sub_logging():
    print("Deleting pub sub distribution folder: {}".format(PUB_SUB_LOGGING_DIST_LOCATION))
    shutil.rmtree(PUB_SUB_LOGGING_DIST_LOCATION)


def run_full_installer(*args):
     run_full_installer_expecting_code(0, *args)

SOPHOS_USER="sophos-spl-user"
SOPHOS_GROUP="sophos-spl-group"


def get_delete_user_cmd():
    with open(os.devnull, "w") as devnull:
        if subprocess.call(["which", "deluser"], stderr=devnull, stdout=devnull) == 0:
            return "deluser"
        if subprocess.call(["which", "userdel"], stderr=devnull, stdout=devnull) == 0:
            return "userdel"


def get_delete_group_cmd():
    with open(os.devnull, "w") as devnull:
        if subprocess.call(["which", "delgroup"], stderr=devnull, stdout=devnull) == 0:
            return "delgroup"
        if subprocess.call(["which", "groupdel"], stderr=devnull, stdout=devnull) == 0:
            return "groupdel"


def Uninstall_SSPL(installdir=None):
    if installdir is None:
        installdir = get_sophos_install()

    if os.path.isdir(installdir):
        p = os.path.join(installdir, "bin", "uninstall.sh")
        if os.path.isfile(p):
            try:
                p = subprocess.Popen([ p, "--force"], stdout="/tmp/install.log", stderr=subprocess.STDOUT)
                if not p.returncode == 0:
                    with open("/tmp/install.log", "r") as f:
                        contents = f.read()
                    logger.info(contents)
            except EnvironmentError as e:
                print("Failed to run uninstaller", e)
        counter = 0
        while counter < 5:
            counter = counter + 1
            try:
                logger.info("try to rm all")
                unmount_all_comms_component_folders(True)
                with open(os.devnull, "w") as devnull:
                    subprocess.check_call(['rm', '-rf', installdir], stderr=devnull, stdout=devnull)
                break
            except Exception as ex:
                logger.error(str(ex))
                unmount_all_comms_component_folders()
                time.sleep(1)


    # Attempts to uninstall based on the env variables in the sophos-spl .service file
    p = subprocess.Popen(["systemctl", "show", "-p", "Environment", "sophos-spl"], stdout=subprocess.PIPE)
    out, err = p.communicate()
    output = out.decode('utf-8')
    installdir_string = "SOPHOS_INSTALL="
    installdir_name = "sophos-spl"
    if installdir_string in output:
        # Cuts the path we want out of the stdout
        start = output.find(installdir_string)+len(installdir_string)
        end = output.find(installdir_name)+len(installdir_name)
        installdir = output[start:end]
        logger.info("Installdir from service files: {}".format(installdir))
        p = os.path.join(installdir,"bin","uninstall.sh")
        if os.path.isfile(p):
            try:
                subprocess.call([p, "--force"])
            except EnvironmentError as e:
                print("Failed to run uninstaller", e)
        subprocess.call(['rm', '-rf', installdir])

    # Find delete user command, deluser on ubuntu and userdel on centos/rhel.
    delete_user_cmd = get_delete_user_cmd()
    logger.debug("Using delete user command:{}".format(delete_user_cmd))
    with open(os.devnull, "w") as devnull:
        subprocess.call([delete_user_cmd, SOPHOS_USER], stderr=devnull)

    # Find delete group command, delgroup on ubuntu and groupdel on centos/rhel.
    delete_group_cmd = get_delete_group_cmd()
    logger.debug("Using delete group command:{}".format(delete_group_cmd))
    if does_group_exist():
        with open(os.devnull, "w") as devnull:
            subprocess.call([delete_group_cmd, SOPHOS_GROUP], stderr=devnull)

def uninstall_sspl_unless_cleanup_disabled(installdir=None):
    if os.path.isfile("/leave_installed"):
        return

    return Uninstall_SSPL(installdir)


def does_group_exist(group=SOPHOS_GROUP):
    try:
        grp.getgrnam(group)
        return True
    except KeyError:
        return False


def verify_group_created(group=SOPHOS_GROUP):
    grp.getgrnam(group)


def verify_user_created(user=SOPHOS_USER):
    pwd.getpwnam(user)


def verify_group_removed(group=SOPHOS_GROUP):
    if does_group_exist(group):
        raise AssertionError("Group %s still exists" % group)


def verify_user_removed(user=SOPHOS_USER):
    try:
        verify_user_created(user)
        raise AssertionError("User %s still exists" % user)
    except Exception as e:
        print(e)


def require_uninstalled(*args):
    installdir = get_sophos_install()
    Uninstall_SSPL(installdir)


def start_system_watchdog():
    logger.info("Start SSPL Service")
    p = subprocess.Popen(['systemctl', 'start', 'sophos-spl'], stderr=subprocess.PIPE, stdout=subprocess.PIPE)
    out, err = p.communicate()
    if out:
        logger.info(out)
    if err:
        logger.info(err)


def _remove_files_recursively(directory_path):
    for fname in os.listdir(directory_path):
        if fname in ['.', '..']:
            continue
        full_path = os.path.join(directory_path, fname)
        if os.path.isfile(full_path):
            try:
                os.remove(full_path)
            except IOError:
                pass
        if os.path.isdir(full_path):
            _remove_files_recursively(full_path)


def require_fresh_startup():
    installdir = get_sophos_install()
    logger.info("Stop SSPL Service")
    p = subprocess.Popen(['systemctl', 'stop', 'sophos-spl'], stderr=subprocess.PIPE, stdout=subprocess.PIPE)
    out, err = p.communicate()
    if out:
        logger.info(out)
    if err:
        logger.info(err)
    mcs_dir = os.path.join(installdir, 'base/mcs')
    logger.info("Clean up mcs files")
    for folder in ['event', 'policy', 'status', 'action']:
        mcs_dir_path = os.path.join(mcs_dir, folder)
        if os.path.exists(mcs_dir_path):
            _remove_files_recursively(mcs_dir_path)
    logs_dir = os.path.join(installdir, 'logs')
    _remove_files_recursively(logs_dir)
    start_system_watchdog()


def get_machine_id_generate_by_python():
    import sys
    supportFilesPath = PathManager.get_support_file_path()
    sys.path.append(supportFilesPath)
    import SXLMachineID as sxl
    return sxl.generateMachineId()


def clean_up_warehouse_temp_dir():
    temp_dir_prefixes = [
        "robot-example-plugin*",
        "robot-event-processsor-plugin*",
        "robot-audit-plugin*"
    ]
    for dir in temp_dir_prefixes:
        dir_list = glob.iglob(os.path.join("/tmp", dir))
        logger.info(dir_list)
        for path in dir_list:
            if os.path.isdir(path):
                shutil.rmtree(path)


def get_plugin_name_for_log_component_name(logname):
    names_pairs = {"sophos_managementagent": "managementagent",
                   "mcs_router": "mcsrouter"}
    return names_pairs.get(logname, logname)

def get_relative_log_path_for_log_component_name(logname):
    relative_paths = {"sophos_managementagent": "logs/base/sophosspl/sophos_managementagent.log",
                      "mcs_router": "logs/base/sophosspl/mcsrouter.log",
                      "updatescheduler": "logs/base/sophosspl/updatescheduler.log",
                      "EventProcessor": "plugins/EventProcessor/log/EventProcessor.log",
                      "AuditPlugin": "plugins/AuditPlugin/log/AuditPlugin.log"

                      }
    relative_path = relative_paths.get(logname, os.path.join("logs/base", logname + '.log'))
    return relative_path


def get_the_loggerconf_file_for(component_name, field, field_value, **kwargs):
    base_template = """
[{}]
{} = {}    
    """
    component_logger = base_template.format(component_name, field, field_value)

    for key, value in list(kwargs.items()):
        component_logger += "\n[{}]\n{}={}\n".format(key, field, value)
    return component_logger


def get_logger_conf_path():
    install_dir = get_sophos_install()
    logger_conf_pattern = os.path.join(install_dir, 'base/etc/logger.conf.')
    files = glob.glob(logger_conf_pattern + '*')
    if files:
        return files[0]
    else:
        return logger_conf_pattern + '0'


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


def get_file_info_for_installation(plugin=None):
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
    SOPHOS_INSTALL = get_sophos_install()
    exclusions = open(os.path.join(PathManager.get_robot_tests_path(), "installer/InstallSet/ExcludeFiles")).readlines()
    exclusions = set(( e.strip() for e in exclusions ))

    # Allows the overriding of this function to work for specific plugins
    if plugin:
        plugin = plugin.lower()
        plugin_dir = "plugins/{}".format(plugin)
        SOPHOS_INSTALL = os.path.join(SOPHOS_INSTALL, plugin_dir)
        if plugin == "mtr":
            exclusions = open(os.path.join(PathManager.get_robot_tests_path(), "mdr_plugin/InstallSet/ExcludeFiles")).readlines()
        elif plugin == "edr":
            exclusions = open(os.path.join(PathManager.get_robot_tests_path(), "edr_plugin/InstallSet/ExcludeFiles")).readlines()
        elif plugin == "liveresponse":
            exclusions = open(os.path.join(PathManager.get_robot_tests_path(), "mdr_plugin/InstallSet/ExcludeFiles")).readlines()
        exclusions = set(( e.strip() for e in exclusions ))

    fullFiles = set()
    fullDirectories = [SOPHOS_INSTALL]

    for (base, dirs, files) in os.walk(SOPHOS_INSTALL):
        for f in files:
            include = True
            for e in exclusions:
                if e in f or e in base:
                    include = False
                    break
            if include:
                fullFiles.add(os.path.join(base, f))
        for d in dirs:
            include = True
            for e in exclusions:
                if e in d or e in base:
                    include = False
                    break
            if include:
                fullDirectories.append(os.path.join(base, d))

    details = []
    symlinks = []
    unixsocket = []

    for f in fullFiles:
        result, s = get_result_and_stat(f)
        if stat.S_ISLNK(s.st_mode):
            if not result.endswith('pyc'):
                symlinks.append(result)
        elif stat.S_ISSOCK(s.st_mode):
            unixsocket.append(result)
        else:
            if not result.endswith('pyc.0'):
                details.append(result)

    key = lambda x : x.replace(".", "").lower()
    symlinks.sort(key=key)
    details.sort(key=key)

    directories = []
    for d in fullDirectories:
        result, s = get_result_and_stat(d)
        directories.append(result)
    directories.sort(key=key)

    return "\n".join(directories), "\n".join(details), "\n".join(symlinks)


def get_file_info_for_installation_debug():
    directories, details, symlinks = get_file_info_for_installation()

    return "||".join(directories.split("\n")), "||".join(details.split("\n")), "||".join(symlinks.split("\n"))


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

    fullFiles = []

    for (base, dirs, files) in os.walk(systemd_path):
        for f in files:
            if not f.startswith("sophos-spl"):
                continue
            p = os.path.join(base, f)
            if os.path.islink(p):
                continue
            fullFiles.append(p)

    results = []
    for p in fullFiles:
        result, s = get_result_and_stat(p)
        results.append(result)
    key = lambda x : x.replace(".", "").replace("-", "").lower()
    results.sort(key=key)

    return "\n".join(results)


def version_ini_file_contains_proper_format_for_product_name(file, product_name):

    name_pattern = "^PRODUCT_NAME = "+product_name+"\n\Z"
    version_pattern = "^PRODUCT_VERSION = ([0-9]*\.)*[0-9]*\n\Z"
    date_pattern = "^BUILD_DATE = [0-9]{4}\-[0-9]{2}\-[0-9]{2}\n\Z"

    patterns = [name_pattern, version_pattern, date_pattern]


    lines = []
    with open(file) as f:
        lines = f.readlines()

    if len(lines) != len(patterns):
        raise AssertionError

    n = 0
    for line in lines:
        if re.match(patterns[n],line) is None:
            logger.info("line = ||{}||".format(line))
            logger.info("{} does not match {}".format(line,patterns[n]))
            raise AssertionError
        n += 1

def component_suite_version_ini_file_contains_proper_format(file):
    """
    DBOS_VERSION = 1.0.0
    OSQUERY_VERSION = 3.3.2
    """

    dbos_pattern = "^SOPHOSMTR_VERSION = ([0-9]*\.)*[0-9]*\n\Z"

    lines = []
    with open(file) as f:
        lines = f.readlines()

    if not re.match(dbos_pattern,lines[0]):
        logger.info("Incorrect format in: {}".format(file))
        raise AssertionError

def SSPL_is_installed():
    installdir = get_sophos_install()
    if os.path.isdir(os.path.join(installdir,"base")):
        if os.path.isfile(os.path.join(installdir,"/bin/wdctl")):
            return True

    return False

def _get_file_content(file_path):
    if os.path.exists(file_path):
        with open(file_path, 'r') as file_handler:
            return file_handler.read()
    return ""

def _information_related_to_process_pid( pid):
    output = ""
    try:
        output = subprocess.check_output("ps -elf | grep ' {} '".format(pid), shell=True)
    except subprocess.CalledProcessError as ex:
        logger.info("Check of process failed for command {} with exit code: {} and output: {}".format(
            ex.cmd, ex.returncode, ex.output))

    return output.decode("utf-8")

def report_on_process(pids):
    """Log the /proc/<pid>/status and ps -elf | grep <pid> for all the pids in the input.
       pids is to be a space separated list of pid numbers. For example:   report_on_process("3456 8979")
    """
    for pid in pids.split(' '):
        try:
            pidn = int(pid)
            proc_status_path=os.path.join('/proc/', str(pidn), 'status')
            logger.info("content of file {}:\n{}".format(proc_status_path, _get_file_content(proc_status_path)))
            related_info = _information_related_to_process_pid(pidn)
            logger.info("ps -elf | grep {} produced: \n {}".format(pidn, related_info))

        except ValueError:
            logger.info("Ignoring input {} as it is not a number".format(pid))

# return a configparser.Section with the following entries: PRODUCT_NAME, PRODUCT_VERSION, BUILD_DATE
def _load_version_file(file_path):
    content = _get_file_content(file_path)
    ini_content = '''[default]
    {}'''.format(content)
    parser = configparser.ConfigParser()
    parser.read_string(ini_content)
    return parser['default']

def _is_valid_upgrade(previous_file_path_1, latest_file_path_2):
    previous_version = _load_version_file(previous_file_path_1)
    latest_version = _load_version_file(latest_file_path_2)
    assert 'PRODUCT_NAME' in previous_version
    assert 'PRODUCT_VERSION' in previous_version
    assert 'BUILD_DATE' in previous_version
    assert 'PRODUCT_NAME' in latest_version
    assert 'PRODUCT_VERSION' in latest_version
    assert 'BUILD_DATE' in latest_version
    # not an upgrade if version is the same
    if (previous_version['BUILD_DATE'] == latest_version['BUILD_DATE'] and
        previous_version['PRODUCT_VERSION'] == latest_version['PRODUCT_VERSION']):
        return False
    if (latest_version['BUILD_DATE'] > previous_version['BUILD_DATE'] and
       latest_version['PRODUCT_VERSION'] != previous_version['PRODUCT_VERSION']):
        return True
    raise AssertionError("The pair is neither in the same release nor an upgrade. Previous={} Latest={}".format(
        previous_version, latest_version
    ))


def check_version_files_report_a_valid_upgrade(previous_ini_files, recent_ini_files):
    if len(previous_ini_files) != len(recent_ini_files):
        raise AssertionError("Invalid input arguments. Require two lists with equal size. Received: 1->{} 2->{}".format(previous_ini_files,recent_ini_files))
    upgrades = [_is_valid_upgrade(previous,latest) for previous,latest in zip(previous_ini_files,recent_ini_files)]
    if not any(upgrades):
        raise AssertionError('No upgrade found in the input VERSION files')


def unmount_all_comms_component_folders(skip_stop_proc=False):
    def _run_proc(args):
        logger.info('Run Command: {}'.format(args))
        p=subprocess.Popen(args, stdout=subprocess.PIPE)
        p.wait()
        stdout, stderr = p.communicate()
        if stdout is None:
            stdout = ''
        if stderr is None:
            stderr = ''
        return stdout, stderr

    def _umount_path(fullpath):

        stdout, stderr = _run_proc(['umount', fullpath])
        if 'not mounted' in stderr: 
            return
        logger.info(stdout)
        logger.info(stderr)

    def _stop_commscomponent():
        stdout, stderr = _run_proc(["/opt/sophos-spl/bin/wdctl", "stop", "commscomponent"])
        if stdout:
            logger.info(stdout)
        if stderr:
            if not 'Watchdog is not running' in stderr:
                logger.info(stderr)


    if not os.path.exists('/opt/sophos-spl/bin/wdctl'):
        return
    # stop the comms component as it could be holding the mounted paths and 
    # would not allow them to be unmounted. 
    counter = 0
    while not skip_stop_proc and counter < 5:
        counter = counter + 1
        stdout, stderr = _run_proc(['pidof', 'CommsComponent'])
        if len(stdout)>1:
            logger.info("Commscomponent running {}".format(stdout))
            _stop_commscomponent()    
            time.sleep(1)
        else:
            logger.info("Skip stop comms componenent")
            break

    dirpath = '/opt/sophos-spl/var/sophos-spl-comms/'
    
    mounted_entries = ['etc/resolv.conf', 'etc/hosts', 'usr/lib', 'usr/lib64', 'lib', 
                        'etc/ssl/certs', 'etc/pki/tls/certs', 'base/mcs/certs']
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
