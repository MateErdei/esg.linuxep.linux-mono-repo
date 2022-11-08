import logging
import os
import shutil
import subprocess
import time

import LogUtils

SOPHOS_INSTALL = "/opt/sophos-spl"
AV_POLICY_BKP = "/tmp/SAV-2_policy_bkp.xml"


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
    shutil.move(os.path.join("/tmp/", "SAV-2_policy.xml"), os.path.join(SOPHOS_INSTALL, "base/mcs/policy/SAV-2_policy.xml"))
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
