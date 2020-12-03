
import os
import pwd
import grp
import subprocess
import sys
from robot.api import logger

try:
    from . import PathManager
except ImportError as ex:
    import PathManager


def install_system_ca_cert(certificate_path):
    script = os.path.join(PathManager.get_resources_path(), "sophos_certs", "InstallCertificateToSystem.sh")
    command = [script, certificate_path]
    logger.info(command)
    process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = process.communicate()
    # logger.info(process.returncode)
    # logger.info(out)
    # logger.info(err)
    if process.returncode != 0:
        logger.info("stdout: {}\nstderr: {}".format(out, err))
        raise OSError("Failed to install \"{}\" to system".format(certificate_path))


def get_cwd_then_change_directory(path):
    cwd = os.getcwd()
    os.chdir(path)
    return cwd


def change_owner(path, user, group):
    uid = pwd.getpwnam(user)[2]
    gid = grp.getgrnam(group)[2]
    os.chown(path, uid, gid)
