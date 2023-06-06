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
MANAGEMENT_AGENT_LOG = os.path.join(SOPHOS_INSTALL, "logs", "base", "sophosspl", "sophos_managementagent.log")

# Configure the plugin you want to monitor
LOG_PATH = os.path.join(SOPHOS_INSTALL, "logs", "base", "sophosspl", "updatescheduler.log")
EXECUTABLE = "UpdateScheduler"
EXECUTABLE_PATH = os.path.join(SOPHOS_INSTALL, "base", "bin", EXECUTABLE)
PLUGIN = "updatescheduler"

COUNTS = {
    "more_than_2_seconds": 0,
    "more_than_0.5_seconds": 0,
    "error": 0,
    "quick_good": 0
}
TOTAL = 0


def wait_for_running(initial_wait, executable, proc, logmark: LogHandler.LogMark):
    try:
        retcode = proc.wait(initial_wait)
        raise AssertionError(f"{executable} exited within {initial_wait} seconds with {retcode}")
    except subprocess.TimeoutExpired:
        pass


def inner_loop(initial_wait, executable, executable_path, log_path):
    global TOTAL, COUNTS
    global EXECUTABLE, LOG_PATH
    iteration = 1
    print(f"Starting {executable} {iteration}")

    while True:
        logmark = LogHandler.LogMark(log_path)
        if log_path != MANAGEMENT_AGENT_LOG:
            ma_mark = LogHandler.LogMark(MANAGEMENT_AGENT_LOG)
        else:
            ma_mark = None
        proc = subprocess.Popen([executable_path])
        wait_for_running(initial_wait, executable, proc, logmark)
        proc.send_signal(signal.SIGTERM)
        start = time.time()
        retcode = proc.wait(60)
        end = time.time()
        duration = end - start
        fmt_end = time.strftime("%Y-%m-%dT%H:%M:%S.", time.gmtime(end)) + str(int((end % 1) * 1000))
        if duration > 2:
            print(f"***** ERROR: {executable} exited with {retcode} after {duration} seconds, iteration {iteration} at {fmt_end}")
            log = logmark.get_contents_unicode()
            print("Log contents: " + log)
            if ma_mark:
                log = ma_mark.get_contents_unicode()
                print(f"MA log contents: {log}")
            COUNTS["more_than_2_seconds"] += 1
        elif duration > 0.5:
            fmt_end = time.strftime("%Y-%m-%dT%H:%M:%S.", time.gmtime(end)) + str(int((end % 1) * 1000))
            print(f"{executable} exited with {retcode} after {duration:.4f} seconds, iteration {iteration} at {fmt_end}")
            log = logmark.get_contents_unicode()
            print(f"{executable} log contents: {log}")
            COUNTS["more_than_0.5_seconds"] += 1
        elif retcode == -15:
            print(f"{executable} failed to setup its signal handler and exited with SIGTERM, iteration {iteration} at {fmt_end}")
        elif retcode != 0:
            print(f"{executable} exited with  {retcode} after {duration} seconds, iteration {iteration} at {fmt_end}")
            log = logmark.get_contents_unicode()
            print("Log contents: " + log)
            COUNTS["error"] += 1
        else:
            print(f"{executable} exited after {duration:.4f} seconds, iteration {iteration} at {fmt_end}")
            COUNTS["quick_good"] += 1

        TOTAL += 1
        iteration += 1


def outer_loop(initial_wait, plugin, executable, executable_path, log_path):
    global COUNTS, TOTAL
    subprocess.run([SOPHOS_INSTALL + "/bin/wdctl", "stop", plugin], check=True)

    try:
        inner_loop(initial_wait, executable, executable_path, log_path)
    except KeyboardInterrupt:
        pass

    for key, count in COUNTS.items():
        percent = 100.0 * count / TOTAL
        print(key, count, percent)

    return 0


def main(argv):
    return outer_loop(2, PLUGIN, EXECUTABLE, EXECUTABLE_PATH, LOG_PATH)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
