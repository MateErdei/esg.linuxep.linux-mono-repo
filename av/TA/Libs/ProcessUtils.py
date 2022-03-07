#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (C) 2021 Sophos Plc, Oxford, England.
# All rights reserved.

import os
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

    # write script out
    script = b"""set pagination 0
thread apply all bt
quit
"""
    # run gdb
    proc = subprocess.Popen([b'gdb', b'/proc/%d/exe' % pid, str(pid)],
                            stdin=subprocess.PIPE,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT)

    output = proc.communicate(script)[0].decode("UTF-8")
    exitcode = proc.wait()

    # Get rid of boilerplate before backtraces
    output = output.split("(gdb)", 1)[-1]

    logger.info("pstack (%d):" % ( exitcode))
    for line in output.splitlines():
        logger.info(line)


def __main(argv):
    print(pidof(argv[1]))
    if wait_for_pid(argv[1], 2) > 0:
        return 0
    return 1


if __name__ == "__main__":
    sys.exit(__main(sys.argv))
