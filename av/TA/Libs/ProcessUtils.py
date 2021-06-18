#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (C) 2021 Sophos Plc, Oxford, England.
# All rights reserved.

import os
import sys
import time


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


def wait_for_pid(executable, timeout=15):
    start = time.time()
    while time.time() < start + timeout:
        pid = pidof(executable)
        if pid > 0:
            return pid
        time.sleep(0.01)
    raise Exception("Unable to find executable: {} in {} seconds".format(executable, timeout))


def __main(argv):
    print(pidof(argv[1]))
    if wait_for_pid(argv[1], 2) > 0:
        return 0
    return 1


if __name__ == "__main__":
    sys.exit(__main(sys.argv))
