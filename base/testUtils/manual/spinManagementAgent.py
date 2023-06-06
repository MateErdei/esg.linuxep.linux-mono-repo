#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright 2023 Sophos Limited. All rights reserved.

import os
import sys

import spinUpdateScheduler


SOPHOS_INSTALL = spinUpdateScheduler.SOPHOS_INSTALL

spinUpdateScheduler.LOG_PATH = spinUpdateScheduler.MANAGEMENT_AGENT_LOG
spinUpdateScheduler.EXECUTABLE = "sophos_managementagent"
spinUpdateScheduler.EXECUTABLE_PATH = os.path.join(SOPHOS_INSTALL, "base", "bin", spinUpdateScheduler.EXECUTABLE)
spinUpdateScheduler.PLUGIN = "managementagent"

PLUGIN = spinUpdateScheduler.PLUGIN
EXECUTABLE = spinUpdateScheduler.EXECUTABLE
EXECUTABLE_PATH = spinUpdateScheduler.EXECUTABLE_PATH
LOG_PATH = spinUpdateScheduler.LOG_PATH


def wait_for_running(initial_wait, executable, proc, logmark):
    logmark.wait_for_log_contains_from_mark("Management Agent running.", timeout=initial_wait)


spinUpdateScheduler.wait_for_running = wait_for_running


def main(argv):
    return spinUpdateScheduler.outer_loop(10, PLUGIN, EXECUTABLE, EXECUTABLE_PATH, LOG_PATH)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
