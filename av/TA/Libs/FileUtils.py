#!/usr/bin/env python3

import os
import shutil
import stat

from robot.api import logger


def copy_file_if_destination_missing(source, destination):
    if os.path.isfile(destination):
        return False
    shutil.copy2(source, destination)
    return True


def ensure_list_appears_once(path, line_to_append):
    assert os.path.isfile(path)
    lines = open(path).readlines()
    output = []
    found = False
    for line in lines:
        if line == line_to_append:
            if found:
                continue
            found = True
        output.append(line)
    if not found:
        output.append(line_to_append)

    if output != lines:
        open(path, "w").writelines(output)
        return True
    return False


def get_file_size_in_mb(path):
    try:
        size = os.path.getsize(path)/(1024*1024)
    except FileNotFoundError:
        size = 0
    logger.info("{}  size {}".format(path, size))
    return size


def copy_file_with_permissions(src, dest):
    if not os.path.exists(src):
        raise Exception(src + " doesnt exist")

    stats = os.lstat(src)
    if stat.S_ISREG(stats.st_mode) != 0:
        shutil.copy(src, dest)
        os.chown(dest, stats.st_uid, stats.st_gid)
        return
    raise Exception("Unable to copy " + src)


def set_old_timestamps(directory):
    """
    Set old timestamps for all files in a directory
    :param directory:
    :return:
    """
    atime = 99999
    mtime = atime

    for (d, dirs, files) in os.walk(directory):
        for f in files:
            os.utime(os.path.join(d, f), times=(atime, mtime))
