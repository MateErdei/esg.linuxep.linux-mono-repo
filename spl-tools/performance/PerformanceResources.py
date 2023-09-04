import logging
import os
import shutil
import subprocess
import time
from enum import IntEnum

import LogUtils

SOPHOS_INSTALL = "/opt/sophos-spl"
AV_POLICY_BKP = "/tmp/SAV-2_policy_bkp.xml"


class Jenkins_Job_Return_Code(IntEnum):
    SUCCESS = 0
    FAILURE = 1
    UNSTABLE = 2


def get_current_unix_epoch_in_seconds():
    return time.time()


def stop_sspl_process(process):
    wdctl_path = os.path.join(SOPHOS_INSTALL, "bin", "wdctl")
    subprocess.run([wdctl_path, "stop", process])


def start_sspl_process(process):
    wdctl_path = os.path.join(SOPHOS_INSTALL, "bin", "wdctl")
    subprocess.run([wdctl_path, "start", process])


def stop_sspl():
    subprocess.run(["systemctl", "stop", "sophos-spl"])


def start_sspl():
    subprocess.run(["systemctl", "start", "sophos-spl"])


def disable_onaccess():
    log_utils = LogUtils.LogUtils()
    this_dir = os.path.dirname(os.path.realpath(__file__))
    mark = log_utils.get_on_access_log_mark()
    shutil.copyfile(os.path.join(SOPHOS_INSTALL, "base/mcs/policy/SAV-2_policy.xml"), AV_POLICY_BKP)
    shutil.copyfile(os.path.join(this_dir, "SAV-2_policy_OA_disabled.xml"), os.path.join("/tmp/", "SAV-2_policy.xml"))
    shutil.move(os.path.join("/tmp", "SAV-2_policy.xml"), os.path.join(SOPHOS_INSTALL, "base/mcs/policy/SAV-2_policy.xml"))
    log_utils.wait_for_on_access_log_contains_after_mark("soapd_bootstrap <> On-access scanning disabled", mark)
    stop_sspl_process("managementagent")


def enable_onaccess():
    log_utils = LogUtils.LogUtils()
    mark = log_utils.get_on_access_log_mark()
    shutil.copyfile(AV_POLICY_BKP, os.path.join(SOPHOS_INSTALL, "base/mcs/policy/SAV-2_policy.xml"))
    start_sspl_process("managementagent")
    log_utils.wait_for_on_access_log_contains_after_mark("soapd_bootstrap <> On-access scanning enabled", mark)


def wait_for_dir_to_exist(dir: str, timeout: int):
    if not os.path.exists(dir):
        logging.info(f"Waiting for dir to be created: {dir}")
    while time.time() < timeout:
        if os.path.exists(dir):
            return
        time.sleep(1)


def wait_for_plugin_to_be_installed(plugin):
    path = f"/opt/sophos-spl/plugins/{plugin}"
    wait_for_dir_to_exist(path, 4000)


def run_safestore_tool_with_args(*args):
    """
    Usage:
    -h               List this help.
    -l               List the content of the SafeStore database.
    -x=<path>        Export the quarantined file, if any, associated with a single
                        object given in objid. The file will be saved in an
                        obfuscated format under the specified path.
    -dbpath=<path>   Specify the path of the SafeStore database.
    -pass=<passkey>  Specify the password for the database. Must be in
                        hexadecimal form.
    -objid=<GUID>    Specify the unique object ID of an item to be restored.
                        Can be specified multiple times.
    -threatid=<GUID> Specify the threat ID belonging to an event. This option
                        will restore all files and registry keys associated with
                        that event.
    """
    env = os.environ.copy()
    env["LD_LIBRARY_PATH"] = os.path.join(SOPHOS_INSTALL, "base", "lib64")

    safestore_db_path = os.path.join(SOPHOS_INSTALL, "plugins", "av", "var", "safestore_db", "safestore.db")
    safestore_db_password_path = os.path.join(SOPHOS_INSTALL, "plugins", "av", "var", "safestore_db", "safestore.pw")
    with open(safestore_db_password_path, "r") as f:
        hex_string = f.read()
    password = hex_string.encode('utf-8').hex()

    cmd = [os.environ["SAFESTORE_TOOL"], f"-dbpath={safestore_db_path}", f"-pass={password}", *args]

    result = subprocess.run(cmd, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

    if result.returncode != 0:
        logging.error(f"Running {cmd} failed with: {result.returncode}, {result.stderr}")
        exit(1)

    return result.stdout.decode()


def get_safestore_db_content_as_dict():
    safestore_db_content = run_safestore_tool_with_args("-l")
    logging.info("Output from SafeStore tool:")
    logging.info(safestore_db_content)

    threats = [{} for i in range(safestore_db_content.count("Threat GUID:"))]
    threat_idx = 0

    # Example output:
    # Name:         MultiStream_HighScore_LGREP.jar
    # Location:     /root/performance/malware_for_safestore_tests
    # Type:         file
    # Status:       restored_as
    # Store time:   Thu Dec 15 04:56:33 2022 (1671080193)
    # Threat GUID:  07daf2ed-5b17-55ee-80e6-3de5ff2502c9
    # Threat name:  Mal/Generic-R
    #
    # Object GUID:  c61091a4-0142-4384-df0a-228f2e228c8e
    # Name:         MultiStream_OuterDetCleaned.tar
    # Location:     /root/performance/malware_for_safestore_tests
    # Type:         file
    # Status:       restored_as
    # Store time:   Thu Dec 15 04:56:33 2022 (1671080193)
    # Threat GUID:  c27e13f3-1527-5ef8-a18b-7cb0d88f9f08
    # Threat name:  Mal/Generic-R
    for line in safestore_db_content.split("\n"):
        line_content = line.strip().split(": ")
        if len(line_content) == 1:
            threat_idx += 1
            continue
        if len(threats) == threat_idx:
            break
        threats[threat_idx][line_content[0]] = ':'.join(line_content[1:]).strip()

    return threats


def get_endpoint_id():
    with open(os.path.join(SOPHOS_INSTALL, "base", "etc", "sophosspl", "mcs_policy.config")) as f:
        for line in f.readlines():
            if line.startswith("policy_device_id"):
                return line.split("=")[-1].strip()
