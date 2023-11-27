#!/usr/bin/env python3
# Copyright 2021-2023 Sophos Limited. All rights reserved.

import os
import re
import shutil
import stat
import time
import subprocess
import zipfile

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


def basic_copy_file(src, dest):
    if os.path.isdir(dest):
        dest = os.path.join(dest, os.path.basename(src))
    shutil.copyfile(src, dest)


def write_file_after_delay(dest, content, delay: float):
    logger.info("Opening " + dest)
    with open(dest, "a") as f:
        time.sleep(delay)
        logger.info("Writing " + dest)
        f.seek(0)
        f.truncate()
        f.write(content)
        # f.flush()
    logger.info("Closed " + dest)


def wait_for_file_to_contain(path, expected, timeout=30):
    contents = "NEVER READ!"
    timelimit = time.time() + timeout
    while time.time() < timelimit:
        contents = open(path).read()
        if expected in contents:
            logger.info("Found %s in %s: %s" % (expected, path, contents))
            return
        time.sleep(0.5)
    raise AssertionError("Failed to find %s in %s: last contents: %s" % (expected, path, contents))


def open_and_close_file(path):
    with open(path) as _:
        logger.info("Opened " + path)
    logger.info("Closed " + path)

def zip_file(zippath, filetozip):
    with zipfile.ZipFile(zippath, mode='w') as zipf:
        zipf.write(filetozip)


def create_symlink_if_required(target, destination):
    """
    Create a symlink in destination pointing to target, iff destination doesn't already exist
    :param target:
    :param destination:
    :return:
    """
    if os.path.isfile(destination):
        return
    os.symlink(target, destination)


def _copy_or_link_iconv_libraries_from_lib(chroot, lib_dir):
    src_dir = os.path.join("/", lib_dir)
    dest_dir = os.path.join(chroot, lib_dir)
    if not os.path.isdir(src_dir):
        return

    os.makedirs(dest_dir, mode=0o755, exist_ok=True)
    for lib in ["EUC-JP.so", "SJIS.so", "libJIS.so", "ISO8859-1.so", "UTF-32.so"]:
        src = os.path.join(src_dir, lib)
        if not os.path.isfile(src):
            continue
        dest = os.path.join(dest_dir, lib)
        try:
            os.link(src, dest)
        except EnvironmentError:
            shutil.copy2(src, dest)
        os.chmod(dest, 0o755)


def copy_or_link_iconv_libraries(chroot):
    """
    for LIBDIR in "/usr/lib/x86_64-linux-gnu/gconv" "/usr/lib64/gconv"
    do
        [[ -d "$LIBDIR" ]] || continue
        mkdir -p "$SOPHOS_INSTALL/plugins/av/chroot${LIBDIR}"
        for LIB in EUC-JP.so SJIS.so libJIS.so ISO8859-1.so UTF-32.so
        do
            copy_or_link "${LIBDIR}/${LIB}" "$SOPHOS_INSTALL/plugins/av/chroot${LIBDIR}/"
        done
    done
    chmod -R a+rx "$SOPHOS_INSTALL/plugins/av/chroot/usr"
    :param chroot:
    :return:
    """
    _copy_or_link_iconv_libraries_from_lib(chroot, "usr/lib/x86_64-linux-gnu/gconv")
    _copy_or_link_iconv_libraries_from_lib(chroot, "usr/lib64/gconv")
    _copy_or_link_iconv_libraries_from_lib(chroot, "usr/lib/aarch64-linux-gnu/gconv")
