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
    try:
        stats = os.lstat(src)
        if stat.S_ISREG(stats.st_mode) != 0:
            shutil.copy(src, dest)
            os.chown(dest, stats.st_uid, stats.st_gid)
            return "Is Reg File"
        return "Not Reg File"
    except:
        return "File Doesnt Exist"

