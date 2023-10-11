#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright 2021-2023 Sophos Limited. All rights reserved.
# All rights reserved.

import os
import re
import shutil
import subprocess
import sys
import time

from robot.api import logger


def pidof(executable):
    for d in os.listdir("/proc"):
        p = os.path.join("/proc", d, "exe")
        try:
            dest = os.readlink(p)
        except EnvironmentError:
            # process went away while we were trying to read the exe
            # TOCTOU error if we check for existence of link
            continue
        if dest.startswith(executable):
            return int(d)
    return -1


def pidof_or_fail(executable):
    pid = pidof(executable)
    if pid == -1:
        raise AssertionError("%s is not running" % executable)
    return pid


def wait_for_pid(executable, timeout=15):
    start = time.time()
    while time.time() < start + timeout:
        pid = pidof(executable)
        if pid > 0:
            return pid
        time.sleep(0.01)
    raise AssertionError("Unable to find executable: {} in {} seconds".format(executable, timeout))


def wait_for_no_pid(executable, timeout=15):
    start = time.time()
    while time.time() < start + timeout:
        pid = pidof(executable)
        if pid < 0:
            return
        time.sleep(0.01)
    raise AssertionError("Executable still running: {} after {} seconds".format(executable, timeout))


def wait_for_different_pid(executable, original_pid, timeout=5):
    start = time.time()
    pid = -2
    while time.time() < start + timeout:
        pid = pidof(executable)
        if pid != original_pid and pid > 0:
            return
        time.sleep(0.1)
    if pid == original_pid:
        raise AssertionError("Process {} (exe:{}) still running".format(original_pid, executable))
    raise AssertionError("Executable {} no longer running {}".format(executable, pid))


def dump_threads(executable):
    """
    Run gdb to get thread backtrace for the specified process (for the argument executable)
    """
    pid = pidof(executable)
    if pid == -1:
        logger.info("%s not running" % executable)
        return

    return dump_threads_from_pid(pid)


def dump_threads_from_pid(pid):
    gdb = shutil.which('gdb')
    if gdb is None:
        logger.error("gdb not found!")
        return

    # write script out
    script = b"""set pagination 0
thread apply all bt
"""
    # run gdb
    proc = subprocess.Popen([gdb, b'/proc/%d/exe' % pid, str(pid)],
                            stdin=subprocess.PIPE,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT)
    try:
        output = proc.communicate(script, 10)[0].decode("UTF-8")
    except subprocess.TimeoutExpired:
        proc.kill()
        output = proc.communicate()[0].decode("UTF-8")

    exitcode = proc.wait()

    # Get rid of boilerplate before backtraces
    output = output.split("(gdb)", 1)[-1]

    logger.info("pstack (%d):" % exitcode)
    for line in output.splitlines():
        logger.info(line)


def dump_threads_from_process(process):
    """
    Dump threads from a Handle from Start Process
    https://robotframework.org/robotframework/latest/libraries/Process.html#Start%20Process
    :param process:
    :return:
    """
    return dump_threads_from_pid(process.pid)


def get_nice_value(pid):
    try:
        stat = open("/proc/%d/stat" % pid).read()
        re.sub(r" ([^)]+) ", ".", stat)
        parts = stat.split(" ")
        nice = int(parts[18])
        if nice > 19:
            nice -= 2**32
        return nice

    except EnvironmentError:
        raise AssertionError("Unable to get nice value for %d" % pid)


def check_nice_value(pid, expected_nice):
    """
    Check PID has the correct nice value
    :param pid:
    :param expected_nice:
    :return:
    """
    actual_nice = get_nice_value(pid)
    if actual_nice != expected_nice:
        raise AssertionError("PID: %d has nice value %d rather than %d" % (pid, actual_nice, expected_nice))


def __main(argv):
    pid = pidof(argv[1])
    print(pid)
    if pid > 0:
        print(get_nice_value(pid))
    if wait_for_pid(argv[1], 2) > 0:
        return 0
    return 1


if __name__ == "__main__":
    sys.exit(__main(sys.argv))
