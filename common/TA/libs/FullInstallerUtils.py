#!/usr/bin/env python
# Copyright 2018-2023 Sophos Limited. All rights reserved.

import logging
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
import psutil
import platform

import PathManager
import OSUtils
import tempfile

try:
    from robot.api import logger
    logger.warning = logger.warn
except ImportError:
    logger = logging.getLogger("FullInstallUtils")

from robot.libraries.BuiltIn import BuiltIn
import robot.libraries.BuiltIn

SYSTEM_PRODUCT_TEST_INPUTS = os.environ.get("SYSTEMPRODUCT_TEST_INPUT", default="/tmp/system-product-test-inputs")
BASE_VAGRANT_DIR = "/vagrant/esg.linuxep.linux-mono-repo/base"

# using the subprocess.PIPE can make the robot test to hang as the buffer could be filled up. 
# this auxiliary method ensure that this does not happen. It also uses a temporary file in 
# order not to keep files or other stuff around. 
def run_proc_with_safe_output(args, shell=False):
    logger.debug('Run Command: {}'.format(args))
    with tempfile.TemporaryFile(dir=os.path.abspath('/tmp')) as tmpfile:
        p = subprocess.Popen(args, stdout=tmpfile, stderr=tmpfile, shell=shell)
        p.wait()        
        tmpfile.seek(0)
        output = tmpfile.read().decode()
        return output, p.returncode

def run_proc_with_safe_output_and_custom_env(args,custom_env,shell=False):
    logger.debug('Run Command: {}'.format(args))
    with tempfile.TemporaryFile(dir=os.path.abspath('/tmp')) as tmpfile:
        p = subprocess.Popen(args, stdout=tmpfile, stderr=tmpfile, shell=shell,env=custom_env)
        p.wait()
        tmpfile.seek(0)
        output = tmpfile.read().decode()
        return output, p.returncode

def get_variable(varName, defaultValue=None):
    try:
        return BuiltIn().get_variable_value("${%s}" % varName)
    except robot.libraries.BuiltIn.RobotNotRunningError:
        return os.environ.get(varName, defaultValue)


def get_sophos_install():
    sophos_install = get_variable("SOPHOS_INSTALL")
    if not sophos_install:
        sophos_install = "/opt/sophos-spl"
    logger.debug(f"Sophos install path is {sophos_install}")
    return sophos_install


def newer_file(*files: str) -> str:
    files = [ f for f in files if os.path.isfile(f) ]
    if len(files) == 0:
        return ""
    if len(files) == 1:
        return files[0]

    files = [ (os.path.getctime(f), f) for f in files ]
    files.sort(reverse=True)
    return files[0][1]


def get_full_installer():
    OUTPUT = os.environ.get("OUTPUT")
    BASE_DIST = os.environ.get("BASE_DIST", "/opt/test/inputs/base_sdds")

    logger.debug("Getting installer BASE_DIST: {}, OUTPUT: {}".format(BASE_DIST, OUTPUT))

    paths_tried = []

    if BASE_DIST is not None:
        installer = os.path.join(BASE_DIST, "install.sh")
        if os.path.isfile(installer):
            logger.debug("Using installer from BASE_DIST: {}".format(installer))
            return installer
        paths_tried.append(installer)
    else:
        paths_tried.append("BASE_DIST not set")

    if OUTPUT is not None:
        installer = os.path.join(OUTPUT, "SDDS-COMPONENT", "install.sh")
        if os.path.isfile(installer):
            logger.debug("Using installer from OUTPUT: {}".format(installer))
            return installer
        paths_tried.append(installer)
    else:
        paths_tried.append("OUTPUT not set")

    installer_release = os.path.join(f"{BASE_VAGRANT_DIR}/cmake-build-release/distribution/base/install.sh")
    installer_debug = os.path.join(f"{BASE_VAGRANT_DIR}/cmake-build-debug/distribution/base/install.sh")
    output = os.path.join(f"{BASE_VAGRANT_DIR}/output/SDDS-COMPONENT/install.sh")
    paths_tried.append(installer_release)
    paths_tried.append(installer_debug)
    paths_tried.append(output)
    installer = newer_file(installer_release, installer_debug, output)

    if os.path.isfile(installer):
        logger.debug(f"Using installer {installer}")
        return installer

    raise Exception("get_full_installer - Failed to find installer at: " + str(paths_tried))


def get_folder_with_installer():
    return os.path.dirname(get_full_installer())


def get_plugin_sdds(plugin_name, environment_variable_name, candidates=None):
    # Check if an environment variable has been set pointing to the example plugin sdds
    SDDS_PATH = os.environ.get(environment_variable_name)
    logger.info(f"SDDS_PATH={SDDS_PATH}")
    env_path = ""
    if SDDS_PATH is not None:
        env_path = SDDS_PATH
        if os.path.exists(SDDS_PATH):
            logger.info(f"Using plugin sdds defined by {environment_variable_name}: {SDDS_PATH}")
            return SDDS_PATH

    fullpath_candidates = [os.path.abspath(p) for p in candidates]

    for plugin_sdds in fullpath_candidates:
        if os.path.exists(plugin_sdds):
            logger.info(f"Using local plugin {plugin_name} sdds: {plugin_sdds}")
            return plugin_sdds

    raise Exception(f"Failed to find plugin {plugin_name} sdds. Attempted: " + ' '.join(fullpath_candidates) + " " + env_path)

def get_sspl_device_isolation_plugin_sdds():
    candidates = []
    local_path_to_plugin = PathManager.find_local_component_dir_path("di")
    if local_path_to_plugin:
        candidates.append(os.path.join(local_path_to_plugin))
    return get_plugin_sdds("Sophos Linux Device Isolation", "SSPL_DEVICE_ISOLATION_PLUGIN_SDDS", candidates)


def get_sspl_live_response_plugin_sdds():
    candidates = []
    local_path_to_plugin = PathManager.find_local_component_dir_path("esg.winep.liveterminal")
    if local_path_to_plugin:
        candidates.append(os.path.join(local_path_to_plugin))
        candidates.append(os.path.join(local_path_to_plugin, "build64/sdds"))
        candidates.append(os.path.join(local_path_to_plugin, "cmake-build-debug/sdds"))
    candidates.append(os.path.join(SYSTEM_PRODUCT_TEST_INPUTS, "liveterminal"))
    return get_plugin_sdds("SSPL LiveResponse Plugin", "SSPL_LIVERESPONSE_PLUGIN_SDDS", candidates)

def get_sspl_edr_plugin_sdds():
    candidates = []
    local_path_to_plugin = PathManager.find_local_component_dir_path("edr_sdds")
    if local_path_to_plugin:
        candidates.append(os.path.join(local_path_to_plugin))
        candidates.append(os.path.join(local_path_to_plugin, "build64/sdds"))
        candidates.append(os.path.join(local_path_to_plugin, "cmake-build-debug/sdds"))
    return get_plugin_sdds("SSPL EDR Plugin", "SSPL_EDR_PLUGIN_SDDS", candidates)

def get_sspl_runtimedetections_plugin_sdds():
    candidates = [os.path.join(SYSTEM_PRODUCT_TEST_INPUTS, "sspl-runtimedetections-plugin")]
    local_path_to_plugin = PathManager.find_local_component_dir_path("esg.linuxep.sspl-plugin-runtimedetections-component")
    if local_path_to_plugin:
        candidates.append(os.path.join(local_path_to_plugin))
        candidates.append(os.path.join(local_path_to_plugin, "build64/sdds"))
        candidates.append(os.path.join(local_path_to_plugin, "cmake-build-debug/sdds"))
    return get_plugin_sdds("SSPL Runtime Detections Plugin", "SSPL_RUNTIMEDETECTIONS_PLUGIN_SDDS", candidates)

def get_sspl_event_journaler_plugin_sdds():
    candidates = []
    local_path_to_plugin = PathManager.find_local_component_dir_path("eventjournaler")
    if local_path_to_plugin:
        candidates.append(os.path.join(local_path_to_plugin))
        candidates.append(os.path.join(local_path_to_plugin, "build64/sdds"))
        candidates.append(os.path.join(local_path_to_plugin, "cmake-build-debug/sdds"))
        candidates.append(os.path.join(local_path_to_plugin, "output/SDDS-COMPONENT"))
    return get_plugin_sdds("SSPL Event Journaler Plugin", "SSPL_EVENT_JOURNALER_PLUGIN_SDDS", candidates)

def get_sspl_response_actions_plugin_sdds():
    RA_DIST = os.environ.get("RA_DIST")

    if RA_DIST is not None:
        installer = os.path.join(RA_DIST, "install.sh")
        if os.path.isfile(installer):
            logger.debug("Using installer from RA_DIST: {}".format(installer))
            return RA_DIST

    candidates = []
    local_path_to_plugin = PathManager.find_local_component_dir_path(BASE_VAGRANT_DIR)
    if local_path_to_plugin:
        candidates.append(os.path.join(local_path_to_plugin, "cmake-build-release/distribution/ra"))
        candidates.append(os.path.join(local_path_to_plugin, "cmake-build-debug/distribution/ra"))
    return get_plugin_sdds("SSPL Response Actions Plugin", "SSPL_RA_PLUGIN_SDDS", candidates)

def get_sspl_anti_virus_plugin_sdds():
    candidates = []
    local_path_to_plugin = PathManager.find_local_component_dir_path("av")
    if local_path_to_plugin:
        candidates.append(os.path.join(local_path_to_plugin, "build64/sdds"))
        candidates.append(os.path.join(local_path_to_plugin, "cmake-build-debug/sdds"))
        candidates.append(os.path.join(local_path_to_plugin, "output/SDDS-COMPONENT"))
    return get_plugin_sdds("SSPL AV Plugin", "SSPL_ANTI_VIRUS_PLUGIN_SDDS", candidates)


def setup_av_install():
    path_to_sdds = get_sspl_anti_virus_plugin_sdds()
    SDDS_PATH = os.environ.get("SYSTEMPRODUCT_TEST_INPUT")

    if SDDS_PATH is not None:
        path_to_vdl_data = SDDS_PATH + "/vdl"
        path_to_model_data = SDDS_PATH + "/ml_model"
        path_to_localrep_data = SDDS_PATH + "/local_rep"
    else:
        path_to_vdl_data = os.path.join(SYSTEM_PRODUCT_TEST_INPUTS, "vdl")
        path_to_model_data = os.path.join(SYSTEM_PRODUCT_TEST_INPUTS, "ml_model")
        path_to_localrep_data = os.path.join(SYSTEM_PRODUCT_TEST_INPUTS, "local_rep")
    logger.info(f"path_to_vdl_data={path_to_vdl_data}")
    logger.debug(f"path_to_model_data={path_to_model_data}")
    logger.info(f"path_to_localrep_data={path_to_localrep_data}")
    full_av_sdds = os.path.join(SYSTEM_PRODUCT_TEST_INPUTS, "av-sdds")
    if os.path.exists(full_av_sdds):
        shutil.rmtree(full_av_sdds)
    shutil.copytree(path_to_sdds, full_av_sdds)

    shutil.copytree(path_to_vdl_data, os.path.join(full_av_sdds,"files/plugins/av/chroot/susi/update_source/vdl"))
    shutil.copytree(path_to_model_data, os.path.join(full_av_sdds,"files/plugins/av/chroot/susi/update_source/model"))
    shutil.copytree(path_to_localrep_data, os.path.join(full_av_sdds,"files/plugins/av/chroot/susi/update_source/reputation"))

    return full_av_sdds

def get_xml_node_text(node):
    return "".join(t.nodeValue for t in node.childNodes if t.nodeType == t.TEXT_NODE)

def run_full_installer_expecting_code(expected_code, *args):
    installer = get_full_installer()
    logger.info("Installer path: " + str(installer))
    return run_full_installer_from_location_expecting_code(installer, expected_code, *args)

def run_full_installer_with_truncated_path( *args):
    # Robot passes in strings.
    install_script_location = get_full_installer()
    arg_list = ["bash", install_script_location]
    arg_list += list(args)
    logger.info("Run installer: {}".format(arg_list))
    my_env = os.environ
    my_env["PATH"] = "/home/pair/.local/bin:/usr/local/sbin:/usr/local/bin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games:/snap/bin"
    output, actual_code = run_proc_with_safe_output_and_custom_env(arg_list,my_env)
    logger.debug(output)
    if actual_code != 0:
        logger.info(output)
        raise AssertionError("Installer exited with {} rather than {}".format(actual_code, 0))

    return output

def run_full_installer_from_location_expecting_code(install_script_location, expected_code, *args):
    # Robot passes in strings.
    expected_code = int(expected_code)
    arg_list = ["bash", install_script_location]
    arg_list += list(args)
    logger.debug(f"Env Variables: {os.environ}")
    logger.info(f"Run installer: {arg_list}")
    output, actual_code = run_proc_with_safe_output(arg_list)
    logger.debug(output)
    if actual_code != expected_code:
        logger.info(output)
        raise AssertionError(f"Installer exited with {actual_code} rather than {expected_code}")

    return output

def filter_trace_from_installer_output(console_output):
    lines = console_output.split("\n")
    filtered_lines = []
    for line in lines:
        if len(line) > 0 and line[0] != "+":
            filtered_lines.append(line)

    return "\n".join(filtered_lines)

def validate_incorrect_glibc_console_output(console_output):
    filtered_console_output = filter_trace_from_installer_output(console_output)

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

def remove_user(delete_user_cmd, user):
    output, retCode = run_proc_with_safe_output([delete_user_cmd, user])    
    if retCode != 0:
        logger.info(output)
        processes_to_kill = [ {"pid": process.pid, "name": process.name()} for process in psutil.process_iter() if process.username() == user]
        for process in processes_to_kill:
            logger.info(f"Killing {process['name']}")
            output, retCode = run_proc_with_safe_output(["kill", str(process["pid"])])
            if retCode != 0:
                logger.info(output)


def get_delete_group_cmd():
    with open(os.devnull, "w") as devnull:
        if subprocess.call(["which", "delgroup"], stderr=devnull, stdout=devnull) == 0:
            return "delgroup"
        if subprocess.call(["which", "groupdel"], stderr=devnull, stdout=devnull) == 0:
            return "groupdel"


def Uninstall_SSPL(installdir=None, replaceUninstaller=False):
    if installdir is None:
        installdir = get_sophos_install()
    uninstaller_executed = False
    counter = 0
    if os.path.isdir(installdir):
        p = os.path.join(installdir, "bin", "uninstall.sh")
        if os.path.isfile(p):
            if replaceUninstaller:
                # Only replace the uninstaller if the original exists!
                os.environ["SOPHOS_INSTALL"] = installdir
                p = os.path.join(PathManager.get_support_file_path(), "DebugUninstall.sh")
                assert os.path.isfile(p)
            start_time = time.time()
            try:
                contents, returncode = run_proc_with_safe_output(['bash', '-x', p, '--force'])
                uninstaller_executed = True
            except EnvironmentError as e:
                logger.error("Failed to run uninstaller: %s" % str(e))
                contents = str(e)
                returncode = -1
            end_time = time.time()
            uninstall_time = (end_time - start_time)
            if uninstall_time > 60:
                logger.info(contents)
            if returncode != 0:
                logger.info(contents)
                processes, processes_returncode = run_proc_with_safe_output(["ps", "-ef"])
                logger.info(f"processes:")
                logger.info(f"{processes}\n")

                with open("/etc/passwd", 'r') as f:
                    passwd = f.read()
                    logger.info(f"users:")
                    logger.info(f"{passwd}\n")

                with open("/etc/group", 'r') as f:
                    group = f.read()
                    logger.info(f"groups:")
                    logger.info(f"{group}\n")
        while counter < 5 and os.path.exists(installdir):
            counter += 1
            try:
                logger.info("try to rm all")
                unmount_all_comms_component_folders(True)
                output, returncode = run_proc_with_safe_output(['rm', '-rf', installdir])
                if returncode != 0:
                    logger.error(output)
                    time.sleep(1)
                    # try to unmount leftover tmpfs
                    if "Device or resource busy" in output:
                        output, returncode = run_proc_with_safe_output("mount | grep \"tmpfs on /opt/sophos-spl/\" | awk '{print $3}' | xargs umount", shell=True)
                        logger.info(output)
            except Exception as ex:
                logger.error(str(ex))
                time.sleep(1)

    # Attempts to uninstall based on the env variables in the sophos-spl .service file
    output, returncode = run_proc_with_safe_output(["systemctl", "show", "-p", "Environment", "sophos-spl"])
    installdir_string = "SOPHOS_INSTALL="
    installdir_name = "sophos-spl"
    if installdir_string in output:
        # Cuts the path we want out of the stdout
        start = output.find(installdir_string)+len(installdir_string)
        end = output.find(installdir_name)+len(installdir_name)
        installdir = output[start:end]
        logger.info(f"Installdir from service files: {installdir}")
        p = os.path.join(installdir, "bin", "uninstall.sh")
        if os.path.isfile(p):
            try:
                subprocess.call([p, "--force"])
            except EnvironmentError as e:
                print("Failed to run uninstaller", e)
        subprocess.call(['rm', '-rf', installdir])

    # Find delete user command, deluser on ubuntu and userdel on centos/rhel.
    delete_user_cmd = get_delete_user_cmd()
    logger.debug(f"Using delete user command:{delete_user_cmd}")

    # Find delete group command, delgroup on ubuntu and groupdel on centos/rhel.
    delete_group_cmd = get_delete_group_cmd()
    logger.debug(f"Using delete group command:{delete_group_cmd}")
    counter2 = 0 
    time.sleep(0.2)
    while does_group_exist() and counter2 < 5:
        logger.info(f"Removing group, it should have already been removed by now. Counter: {counter2}")
        counter2 += 1
        for user in ('sophos-spl-user', 'sophos-spl-network', 'sophos-spl-local', 'sophos-spl-updatescheduler',
                     'sophos-spl-av', 'sophos-spl-threat-detector'):
            if does_user_exist(user):
                remove_user(delete_user_cmd, user)

        out, code = run_proc_with_safe_output([delete_group_cmd, SOPHOS_GROUP])
        if code != 0:
            logger.info(out)

        time.sleep(0.5)
    if uninstaller_executed and (counter > 0 or counter2 > 0):
        logger.info(_get_file_content('/tmp/install.log'))
        message = f"Uninstaller failed to properly clean everything. Attempt to remove /opt/sophos-spl {counter} " \
                  f"attempt to remove group {counter2}"
        logger.warn(message)
        raise RuntimeError(message)
    

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

def does_user_exist(user):
    try:
        pwd.getpwnam(user)
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
    Uninstall_SSPL(installdir, True)


def start_system_watchdog():
    logger.info("Start SSPL Service")
    out, _ = run_proc_with_safe_output(['systemctl', 'start', 'sophos-spl'])
    logger.info(out)


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


def get_machine_id_generate_by_python():
    import sys
    supportFilesPath = PathManager.get_support_file_path()
    sys.path.append(supportFilesPath)
    import SXLMachineID as sxl
    return sxl.generateMachineId()


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
    logger_conf = os.path.join(install_dir, 'base/etc/logger.conf')

    return logger_conf


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
    exclusions = []
    SOPHOS_INSTALL = get_sophos_install()
    # Allows the overriding of this function to work for specific plugins
    if plugin:
        plugin = plugin.lower()
        plugin_dir = "plugins/{}".format(plugin)
        SOPHOS_INSTALL = os.path.join(SOPHOS_INSTALL, plugin_dir)
        if plugin == "liveresponse" or plugin == "eventjournaler" or plugin == "deviceisolation":
            exclusions = open(os.path.join(PathManager.get_robot_tests_path(), "RobotTests/InstallSet/ExcludeFiles")).readlines()
        elif plugin == "edr":
            exclusions = open(os.path.join(PathManager.get_robot_tests_path(), "RobotScripts/InstallSet/ExcludeFiles")).readlines()
    else:
        exclusions = open(os.path.join(PathManager.get_robot_tests_path(), "tests/installer/InstallSet/ExcludeFiles")).readlines()
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

    systemd_paths = []
    if os.path.isdir("/usr/lib/systemd/system/"):
        systemd_paths.append("/usr/lib/systemd/system/")
    if os.path.isdir("/lib/systemd/system/"):
        systemd_paths.append("/lib/systemd/system/")
    if os.path.isdir("/etc/systemd/system/multi-user.target.wants/"):
        systemd_paths.append("/etc/systemd/system/multi-user.target.wants/")

    fullFiles = []
    for systemd_path in systemd_paths:
        for (base, dirs, files) in os.walk(systemd_path):
            for f in files:
                if f.startswith("sophos-spl"):
                    fullFiles.append(os.path.join(base, f))

    results = []
    for p in fullFiles:
        result, s = get_result_and_stat(p)
        results.append(result)
    key = lambda x : x.replace(".", "").replace("-", "").lower()
    results.sort(key=key)

    return "\n".join(results)

def replace_version(old_version, new_version, base_dist):
    sdds_import_path = os.path.join(base_dist, "SDDS-Import.xml")
    version_file_path = os.path.join(base_dist, "files/base/VERSION.ini")
    OSUtils.replace_string_in_file(old_version, new_version, sdds_import_path)
    OSUtils.replace_string_in_file(old_version, new_version, version_file_path)

def version_ini_file_contains_proper_format_for_product_name(file, product_name):

    name_pattern = "^PRODUCT_NAME = "+product_name+"\n\Z"
    version_pattern = "^PRODUCT_VERSION = ([0-9]*\.)*[0-9]*\n\Z"
    date_pattern = "^BUILD_DATE = [0-9]{4}\-[0-9]{2}\-[0-9]{2}\n\Z"
    git_commit_pattern = "^COMMIT_HASH = [0-9a-fA-F]{40}\n\Z"

    patterns = [name_pattern, version_pattern, date_pattern, git_commit_pattern]


    lines = []
    with open(file) as f:
        lines = f.readlines()

    n = 0
    for pattern in patterns:
        if re.match(pattern, lines[n]) is None:
            logger.info(f"line = ||{lines[n]}||")
            raise AssertionError(f"VERSION.ini field {lines[n]} does not match {pattern}")
        n += 1


def SSPL_is_installed():
    installdir = get_sophos_install()
    if os.path.isdir(os.path.join(installdir, "base")):
        if os.path.isfile(os.path.join(installdir, "bin/wdctl")):
            logger.info("SPL is installed")
            return True
    logger.info("SPL is not installed")
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


def check_watchdog_service_file_has_correct_kill_mode():
    if os.path.isfile("/lib/systemd/system/sophos-spl.service"):
        path = "/lib/systemd/system/sophos-spl.service"
    elif os.path.isfile("/usr/lib/systemd/system/sophos-spl.service"):
        path = "/usr/lib/systemd/system/sophos-spl.service"
    else:
        raise AssertionError('Watchdog service file not installed')

    content = _get_file_content(path)
    if "KillMode=mixed" not in content:
        raise AssertionError(f'KillMode not set to mixed: {path}: {content}')

def umount_path(fullpath):
    stdout, code = run_proc_with_safe_output(['umount', fullpath])
    if 'not mounted' in stdout:
        return 0
    if code != 0:
        logger.info(stdout)
    return code

def umount_with_retry(directory_path,timeout=30):
    counter = 0
    while counter < timeout:
        ret = umount_path(directory_path)
        if ret == 0:
            return 0
        counter += 1
        time.sleep(1)
    return ret

def umount_with_retry_fallback_to_lazy(directory_path,timeout=30):
    ret = umount_with_retry(directory_path, timeout)
    if ret != 0:
        logger.warning(f"Doing a lazy unmount as we could not do a normal umount on {directory_path} after {timeout} secs")
        run_proc_with_safe_output(['umount', "-l", directory_path])
        # we want to fail the test if we need to fallback here
        return ret
    return 0

def unmount_all_comms_component_folders(skip_stop_proc=False):

    def _stop_commscomponent():
        stdout, code = run_proc_with_safe_output(["/opt/sophos-spl/bin/wdctl", "stop", "commscomponent"])
        if code != 0 and not 'Watchdog is not running' in stdout:
            logger.info(stdout)

    if os.path.exists('/opt/sophos-spl/bin/wdctl'):

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
    umount_failed = False
    mounted_entries = ['etc/resolv.conf', 'etc/hosts', 'usr/lib', 'usr/lib64', 'lib',
                        'etc/ssl/certs', 'etc/pki/tls/certs', 'etc/pki/ca-trust/extracted', 'base/mcs/certs','base/remote-diagnose/output']
    for entry in mounted_entries:
        try:
            fullpath = os.path.join(dirpath, entry)
            if not os.path.exists(fullpath):
                continue
            ret = umount_with_retry_fallback_to_lazy(fullpath, 5)
            if ret != 0:
                umount_failed = True
            if os.path.isfile(fullpath):
                os.remove(fullpath)
            else:
                shutil.rmtree(fullpath)
        except Exception as ex:
            umount_failed = True
            logger.error(str(ex))

    if umount_failed:
        raise AssertionError("umounting comms failed")


def file_info_with_custom_install_directory(file_info_path, custom_install_dir):
    with open(file_info_path) as file:
        return file.read().replace("/opt/sophos-spl", custom_install_dir)


def adjust_base_install_set_expected_file_info_for_platform(expected, install_dir="/opt/sophos-spl"):
    arch = platform.uname()[4]
    if arch == "x86_64":
        expected = expected.split("\n")
        for i, line in enumerate(expected[:]):
            # These libraries are not present on the ARM64 distribution of pycryptodome
            if line.endswith("_raw_aes.abi3.so.0"):
                expected.insert(i + 1, f"o750, sophos-spl-group, root, {install_dir}/base/lib/python3.11/site-packages/Crypto/Cipher/_raw_aesni.abi3.so.0")
            if line.endswith("_BLAKE2s.abi3.so.0"):
                expected.insert(i + 2, f"o750, sophos-spl-group, root, {install_dir}/base/lib/python3.11/site-packages/Crypto/Hash/_ghash_clmul.abi3.so.0")
                break
        expected = "\n".join(expected)
    return expected


def adjust_base_install_set_expected_symbolic_link_info_for_platform(expected, install_dir="/opt/sophos-spl"):
    arch = platform.uname()[4]
    if arch == "x86_64":
        expected = expected.split("\n")
        for i, line in enumerate(expected[:]):
            # These libraries are not present on the ARM64 distribution of pycryptodome
            if line.endswith("_raw_aes.abi3.so"):
                expected.insert(i + 1, f"o777, sophos-spl-group, root, {install_dir}/base/lib/python3.11/site-packages/Crypto/Cipher/_raw_aesni.abi3.so")
            if line.endswith("_BLAKE2s.abi3.so"):
                expected.insert(i + 2, f"o777, sophos-spl-group, root, {install_dir}/base/lib/python3.11/site-packages/Crypto/Hash/_ghash_clmul.abi3.so")
                break
        expected = "\n".join(expected)
    return expected
