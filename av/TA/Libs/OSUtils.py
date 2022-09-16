import os
import pwd
import grp
import subprocess
import sys
import psutil
import pathlib

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


def create_symlink(src, dst):
    os.symlink(src, dst)


def get_amount_written_to_disk(path):
    disc = find_sdiskpart(path)
    assert disc is not None, "Failed to identify disc"
    split_disc = disc.__str__().split("/")

    with open("/proc/diskstats", "r") as f:
        lines = f.readlines()
        for line in lines:
            if split_disc[len(split_disc)-1] in line:
                split_line = line.split(" ")
                return int(split_line[len(split_line)-1])


def find_sdiskpart(path):
    while not os.path.ismount(path):
        path = os.path.dirname(path)

    path = pathlib.Path(path).resolve()

    for entry in psutil.disk_partitions():
        if entry.mountpoint == path.__str__():
            disc = pathlib.Path(entry.device).resolve()
            return disc

def get_robot_pid():
    return os.getpid()


def generate_only_open_event(path):
    assert os.path.isfile(path)
    f = open(path, 'r')
