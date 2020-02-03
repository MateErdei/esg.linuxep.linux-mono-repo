import os
import datetime
import subprocess
from robot.api import logger
from robot.libraries.BuiltIn import BuiltIn
from BaseInfo import get_value_from_ini_file

def install_fake_stage1_dark_bytes(path="/opt"):
    install_root = os.path.join(path, "dbhs")
    uninstall_directory = os.path.join(install_root, "uninstall")
    uninstall_script_path = os.path.join(uninstall_directory, "uninstall")
    fake_script = "rm -rf {}".format(install_root)

    os.makedirs(uninstall_directory)
    with open(uninstall_script_path, "w") as file:
        file.write(fake_script)
    os.chmod(uninstall_script_path, 777)

def convert_epoch_time_to_UTC_ISO_8601(epoch_time):
    d = datetime.datetime.utcfromtimestamp(int(epoch_time))
    return d.isoformat() + "Z"

def get_mtr_version():
    version_location = os.path.join(BuiltIn().get_variable_value("${SOPHOS_INSTALL}"), "plugins", "mtr", "VERSION.ini")
    try:
        return get_value_from_ini_file(version_location, "PRODUCT_VERSION")
    except Exception as ex:
        logger.info("Could not find version - Returning empty string, error: {}".format(str(ex)))
        return ""
