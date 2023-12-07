#!/bin/env python3

import os
import subprocess
import sys
import time

# manual = os.path.dirname(__file__)
# TA = os.path.dirname(manual)
# edr = os.path.dirname(TA)
# lmono = os.path.dirname(edr)
# Libs = os.path.join(lmono, "common", "TA", "libs")
# sys.path.append(Libs)
# import LogHandler


INST = "/opt/sophos-spl"
WDCTL = os.path.join(INST, "bin/wdctl")
LOG = os.path.join(INST, "plugins", "edr", "log", "edr.log")


def stop_edr(delay):
    start = time.time()
    subprocess.run([WDCTL, "stop", "edr"])
    end = time.time()
    duration = end - start
    if duration > 20:
        print(f"ERROR: stop took {duration} seconds to stop for a run delay of {delay}")
        raise Exception(f"Stop took {duration} seconds for a delay of {delay}")
    else:
        print(f"Stopped EDR in {duration} for delay {delay}")


def start_edr():
    subprocess.run([WDCTL, "start", "edr"])


def test(delay):
    stop_edr(delay)
    start_edr()
    time.sleep(delay)


def main(argv):
    if len(argv) == 2:
        delay = float(argv[1])
        while True:
            test(delay)
    else:
        delay = 0.1
        while True:
            for _ in range(5):
                test(delay)
            delay *= 2


sys.exit(main(sys.argv))
