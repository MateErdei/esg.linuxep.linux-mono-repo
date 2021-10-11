import logging
import os
import subprocess
import time

SOPHOS_INSTALL = "/opt/sophos-spl"


def get_current_unix_epoch_in_seconds():
    return time.time()


def stop_sspl_process(process):
    wdctl_path = os.path.join(SOPHOS_INSTALL, "bin", "wdctl")
    subprocess.run([wdctl_path, "stop", process])


def start_sspl_process(process):
    wdctl_path = os.path.join(SOPHOS_INSTALL, "bin", "wdctl")
    subprocess.run([wdctl_path, "start", process])


def wait_for_dir_to_exist(dir: str, timeout: int):
    if not os.path.exists(dir):
        logging.info("Waiting for dir to be created: {}".format(dir))
    while time.time() < timeout:
        if os.path.exists(dir):
            return
        time.sleep(1)


def wait_for_plugin_to_be_installed(plugin):
    edr_path = f"/opt/sophos-spl/plugins/{plugin}"
    wait_for_dir_to_exist(edr_path, 4000)
