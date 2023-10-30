#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright 2021-2023 Sophos Limited. All rights reserved.

import os
import re
import shutil
import signal
import sys
import subprocess
import PathManager
import psutil
import time

import robot.api.logger as logger


class ProcessUtils(object):
    def spawn_sleep_process(self, process_name="PickYourPoison", run_from_location="/tmp"):
        pick_your_poison_path = os.path.join(PathManager.ROBOT_ROOT_PATH, "SystemProductTestOutput", "PickYourPoison")
        if not os.path.isfile(pick_your_poison_path):
            raise AssertionError("failed to find pick your poison executable")
        destination = os.path.join(run_from_location, process_name)
        shutil.copy(pick_your_poison_path, destination)

        process = subprocess.Popen([destination, "--run"])
        os.remove(destination)

        return process.pid, process_name, destination

    def get_process_memory(self, pid):
        # Deal with robot passing pid as string.
        if len(pid) == 0:
            raise AssertionError("get_process_memory called with no pid")
        pids = pid.split("\n")
        assert len(pids) > 0
        if len(pids) > 1:
            logger.error("Multiple PIDs provided to get_process_memory: %s" % str(pids))
        pid = int(pids[0])
        process = psutil.Process(pid)

        # in bytes
        return process.memory_info().rss

    # Robot run process and start process don't seem to work well in some cases, they either interfere with each
    # other of just hang. So here are a couple simple run process functions to call from tests.
    def run_process_in_background(self, proc: str, *args) -> int:
        command = [proc]
        for arg in args:
            command.append(arg)
        process = subprocess.Popen(command)
        return process.pid

    def run_and_wait_for_process(self, proc: str, *args) -> tuple:
        command = [proc]
        for arg in args:
            command.append(arg)
        process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = process.communicate()
        rc = process.wait()
        return rc, stdout, stderr

    def kill_process(self, pid, signal_to_send=signal.SIGINT):
        os.kill(pid, signal_to_send)
        wait_until = int(time.time()) + 30
        while psutil.pid_exists(pid) and int(time.time()) < wait_until:
            time.sleep(1)

        # kill the process if it hasn't finished gracefully
        if psutil.pid_exists(pid):
            print(f"The pid {pid} did not shutdown after sending a {signal_to_send} signal, sending SIGKILL")
            os.kill(pid, signal.SIGKILL)

    def pidof(self, executable):
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

    def pidof_or_fail(self, executable, timeout=0):
        pid = self.pidof(executable)
        if pid >= 0:
            return pid

        if timeout > 0:
            start = time.time()
            while time.time() < start + timeout:
                time.sleep(0.1)
                pid = self.pidof(executable)
                if pid >= 0:
                    return pid

        raise AssertionError("%s is not running" % executable)

    def wait_for_pid(self, executable, timeout=15):
        start = time.time()
        while time.time() < start + timeout:
            pid = self.pidof(executable)
            if pid > 0:
                return pid
            time.sleep(0.01)
        raise AssertionError("Unable to find executable: {} in {} seconds".format(executable, timeout))

    def wait_for_no_pid(self, executable, timeout=15):
        start = time.time()
        while time.time() < start + timeout:
            pid = self.pidof(executable)
            if pid < 0:
                return
            time.sleep(0.01)
        raise AssertionError("Executable still running: {} after {} seconds".format(executable, timeout))

    def wait_for_different_pid(self, executable, original_pid, timeout=5):
        start = time.time()
        pid = -2
        while time.time() < start + timeout:
            pid = self.pidof(executable)
            if pid != original_pid and pid > 0:
                return
            time.sleep(0.1)
        if pid == original_pid:
            raise AssertionError("Process {} (exe:{}) still running".format(original_pid, executable))
        raise AssertionError("Executable {} no longer running {}".format(executable, pid))

    def dump_threads(self, executable):
        """
        Run gdb to get thread backtrace for the specified process (for the argument executable)
        """
        pid = self.pidof(executable)
        if pid == -1:
            logger.info("%s not running" % executable)
            return

        return self.dump_threads_from_pid(pid)

    def dump_threads_from_pid(self, pid):
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

    def dump_threads_from_process(self, process):
        """
        Dump threads from a Handle from Start Process
        https://robotframework.org/robotframework/latest/libraries/Process.html#Start%20Process
        :param process:
        :return:
        """
        return self.dump_threads_from_pid(process.pid)

    def get_nice_value(self, pid):
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

    def check_nice_value(self, pid, expected_nice):
        """
        Check PID has the correct nice value
        :param pid:
        :param expected_nice:
        :return:
        """
        actual_nice = self.get_nice_value(pid)
        if actual_nice != expected_nice:
            raise AssertionError("PID: %d has nice value %d rather than %d" % (pid, actual_nice, expected_nice))
