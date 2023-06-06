#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright 2023 Sophos Limited. All rights reserved.

import os
import signal
import subprocess
import sys
import time


manual_dir = os.path.dirname(os.path.realpath(__file__))
testUtils_dir = os.path.dirname(manual_dir)
libs_dir = os.path.join(testUtils_dir, "libs")
sys.path.append(libs_dir)

import LogHandler

SOPHOS_INSTALL = os.environ.get("SOPHOS_INSTALL", "/opt/sophos-spl")
os.environ['SOPHOS_INSTALL'] = SOPHOS_INSTALL
LOG_PATH = os.path.join(SOPHOS_INSTALL, "logs", "base", "sophosspl", "updatescheduler.log")
MANAGEMENT_AGENT_LOG = os.path.join(SOPHOS_INSTALL, "logs", "base", "sophosspl", "sophos_managementagent.log")
EXECUTABLE = "UpdateScheduler"
EXECUTABLE_PATH = os.path.join(SOPHOS_INSTALL, "base", "bin", EXECUTABLE)
PLUGIN = "updatescheduler"

COUNTS = {
    "more_than_2_seconds" : 0,
    "more_than_0.5_seconds" : 0,
    "error" : 0,
    "quick_good" : 0
}
TOTAL = 0


def inner_loop():
    global TOTAL, COUNTS
    iteration = 1
    print(f"Starting {EXECUTABLE} %d" % iteration)
    INITIAL_WAIT = 2

    while True:
        logmark = LogHandler.LogMark(LOG_PATH)
        ma_mark = LogHandler.LogMark(MANAGEMENT_AGENT_LOG)
        proc = subprocess.Popen([EXECUTABLE_PATH])
        try:
            retcode = proc.wait(INITIAL_WAIT)
            raise AssertionError(f"{EXECUTABLE} exited within {INITIAL_WAIT} seconds with {retcode}")
        except subprocess.TimeoutExpired:
            pass
        proc.send_signal(signal.SIGTERM)
        start = time.time()
        retcode = proc.wait(60)
        end = time.time()
        duration = end - start
        fmt_end = time.strftime("%Y-%m-%dT%H:%M:%S.", time.gmtime(end)) + str(int((end % 1) * 1000))
        if duration > 2:
            print(f"***** ERROR: {EXECUTABLE} exited with {retcode} after {duration} seconds, iteration {iteration} at {fmt_end}")
            log = logmark.get_contents_unicode()
            print("Log contents: " + log)
            log = ma_mark.get_contents_unicode()
            print(f"MA log contents: {log}")
            COUNTS["more_than_2_seconds"] += 1
        elif duration > 0.5:
            fmt_end = time.strftime("%Y-%m-%dT%H:%M:%S.", time.gmtime(end)) + str(int((end % 1) * 1000))
            print(f"{EXECUTABLE} exited with {retcode} after {duration} seconds, iteration {iteration} at {fmt_end}")
            log = logmark.get_contents_unicode()
            print(f"{EXECUTABLE} log contents: {log}")
            COUNTS["more_than_0.5_seconds"] += 1
        elif retcode != 0:
            print(f"{EXECUTABLE} exited with  {retcode} after {duration} seconds, iteration {iteration} at {fmt_end}")
            log = logmark.get_contents_unicode()
            print("Log contents: " + log)
            COUNTS["error"] += 1
        else:
            print(f"{EXECUTABLE} exited after {duration:.3f} seconds, iteration {iteration} at {fmt_end}")
            COUNTS["quick_good"] += 1

        TOTAL += 1
        iteration += 1


def main(argv):
    subprocess.run([SOPHOS_INSTALL + "/bin/wdctl", "stop", PLUGIN], check=True)

    try:
        inner_loop()
    except KeyboardInterrupt:
        pass

    for key, count in COUNTS.items():
        percent = 100.0 * count / TOTAL
        print(key, count, percent)

    return 0


sys.exit(main(sys.argv))
