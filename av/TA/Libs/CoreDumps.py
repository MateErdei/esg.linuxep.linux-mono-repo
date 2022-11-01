#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2022 Sophos Plc, Oxford, England.
# All rights reserved.

import datetime
import os
import re
import resource
import shutil
import six
import subprocess
import sys

from robot.api import logger
from robot.libraries.BuiltIn import BuiltIn


def ensure_text(s, encoding="utf-8", errors="replace"):
    if isinstance(s, six.text_type):
        return s
    if isinstance(s, bytes):
        return s.decode(encoding=encoding, errors=errors)
    return six.text_type(s)


def attempt_backtrace_of_core(filepath):
    proc = subprocess.Popen([b'file', b'--brief', filepath],
                            stdout=subprocess.PIPE,
                            stdin=subprocess.PIPE,
                            stderr=subprocess.STDOUT)
    output = proc.communicate()[0].strip()
    logger.info("'file' output: %s" % ensure_text(output))

    try:
        proc = subprocess.Popen([b'gdb', b'-core', filepath],
                                stdout=subprocess.PIPE,
                                stdin=subprocess.PIPE,
                                stderr=subprocess.STDOUT)
        output = proc.communicate(b"\n".join([
            b"set pagination off",
            b"set trace-commands on",
            b"info sharedlibrary",
            b"thread apply all backtrace",
            b"quit"
        ]))[0]
        logger.info("GDB output: %s" % ensure_text(output))
    except FileNotFoundError:
        logger.info("cannot find gdb")


class CoreDumps(object):
    def __init__(self):
        self.__m_ignore_cores_segfaults = False

    def enable_core_files(self):
        # First set local limit to infinity, to cover product and component tests
        resource.setrlimit(resource.RLIMIT_CORE, (resource.RLIM_INFINITY, resource.RLIM_INFINITY))

        # allow suid programs to dump core (also for processes with elevated capabilities)
        sp = subprocess
        sp.Popen(["sysctl", "fs.suid_dumpable=2"], stdout=sp.PIPE, stderr=sp.STDOUT).wait()

        os.makedirs("/z", exist_ok=True)
        os.chmod("/z", 0o777)  # ensure anyone can write to this directory

        # Configure core_pattern so we can find core files
        with open("/proc/sys/kernel/core_pattern", "wb") as f:
            f.write(b"/z/core-%t-%P-%E")

    def dump_dmesg(self):
        sp = subprocess
        dmesg_process = sp.Popen(["dmesg", "-T"], stdout=sp.PIPE, stderr=sp.STDOUT)
        stdout, stderr = dmesg_process.communicate()
        logger.info("dmesg output: %s" % ensure_text(stdout, encoding="utf-8", errors="replace"))

    def check_dmesg_for_segfaults(self, testname=None):
        sp = subprocess
        dmesg_process = sp.Popen([b"dmesg", b"-T"], stdout=sp.PIPE, stderr=sp.STDOUT)
        stdout = dmesg_process.communicate()[0]
        lines = stdout.splitlines()
        result = []
        previous = []
        segfaults = []
        include_next = 0

        for line in lines:
            line = ensure_text(line, encoding="utf-8", errors="replace")
            previous.append(line)
            previous = previous[-5:]
            if "segfault" in line:
                if include_next <= 0:
                    result.append(previous[-4])
                    result.append(previous[-3])
                    result.append(previous[-2])
                result.append(line)
                segfaults.append(line)
                include_next = 2
            elif include_next > 0:
                include_next -= 1
                result.append(line)

        if len(result) == 0:
            #  no segfaults
            return

        if testname is None:
            testname = BuiltIn().get_variable_value("${TEST NAME}")

        RTD_SEGFAULT_RE = re.compile(r".* error [46] in sophos-subprocess-\d+-exec1 \(deleted\)\[.*")
        CLOUD_SETUP_SEGFAULT_RE = re.compile(r"nm-cloud-setup\[\d+\]: segfault at [0-9a-f]+ ip [0-9a-f]+ sp [0-9a-f]+ error 4 in libglib-2.0.so.*\[[0-9a-f]+\+[0-9a-f]+\]")

        all_segfaults_are_ignored = True
        for segfault in segfaults:
            if RTD_SEGFAULT_RE.match(segfault):
                logger.info("Ignoring segfault from RTD")
                continue
            if CLOUD_SETUP_SEGFAULT_RE.search(segfault):
                # [Tue Oct 18 10:19:41 2022] nm-cloud-setup[697]: segfault at 7ffc00000002 ip 00007f07931576ef sp 00007ffc28f16850 error 4 in libglib-2.0.so.0.6800.4[7f0793134000+90000]
                logger.info("Ignoring segfault from nm-cloud-setup")
                continue
            all_segfaults_are_ignored = False

        stdout = "\n".join(result)
        if stdout:
            logger.error("segfaults: %s" % stdout)

        # Subtract Base from IP to give offset
        IP_RE = re.compile(
            r".*: segfault at [0-9a-f]+ ip ([0-9a-f]+) sp [0-9a-f]+ error \d+ in [^\[]+\[([0-9a-f]+)\+[0-9a-f]+\].*")
        for segfault in segfaults:
            segfault = segfault.strip()
            mo = IP_RE.match(segfault)
            if mo:
                ip = int(mo.group(1), 16)
                base = int(mo.group(2), 16)
                offset = ip - base
                logger.info("IP(%s) - Base(%s) = Offset(%s)" % (hex(ip), hex(base), hex(offset)))
            else:
                logger.info("Failed to match segfault %s against re" % segfault)

        logger.debug("dmesg return code: {}, expecting 0".format(dmesg_process.returncode))
        assert dmesg_process.returncode == 0

        if len(result) > 0:
            BuiltIn().run_keyword("Dump Logs")
            BuiltIn().run_keyword("Dump dmesg")
            # Clear the dmesg logs on a segfault to stop all subsequent tests failing for a single segfault
            logger.debug("Clear dmesg after segfault detected")
            sp.Popen(["dmesg", "-C"]).wait()

            self.check_for_coredumps(testname)  # Do this immediately in case it isn't called

            if self.__m_ignore_cores_segfaults:
                logger.debug("Ignoring segfaults in dmesg")
                return ""
            elif all_segfaults_are_ignored:
                logger.error("Not failing test for ignored segfaults! See LINUXDAR-4903")
            else:
                raise AssertionError("segfault detected in dmesg: %s" % stdout)

        return stdout

    def check_for_coredumps(self, testname=None):
        if testname is None:
            testname = BuiltIn().get_variable_value("${TEST NAME}")
        CORE_DIR = "/z"

        if not os.path.exists(CORE_DIR):
            return

        coredumpnames = []
        for file in os.listdir(CORE_DIR):
            if not file.startswith("core-"):
                continue

            file_path = os.path.join(CORE_DIR, file)
            if not os.path.isfile(file_path):
                continue

            if self.__m_ignore_cores_segfaults:
                logger.debug("Ignoring core dump:", file_path)
                os.unlink(file_path)
                continue

            stat_result = os.stat(file_path)
            timestamp = datetime.datetime.utcfromtimestamp(stat_result.st_mtime)
            logger.error("Found core dump at %s, generated at %s UTC" % (file_path, timestamp))
            coredumpnames.append(file)

            # Attempt to gdb core file
            attempt_backtrace_of_core(file_path)

            self.__copy_to_coredump_dir(file_path, testname, timestamp=timestamp)
            os.remove(file_path)

        if not coredumpnames:
            return

        all_core_dumps_are_rtd = True
        RTD_CORE_DUMP_RE = re.compile(r"core-.*sophos-subprocess-\d+-exec1 \(deleted\).\d+")
        for core in coredumpnames:
            core = ensure_text(core)
            if not RTD_CORE_DUMP_RE.match(core):
                all_core_dumps_are_rtd = False

        if all_core_dumps_are_rtd:
            logger.warn("Found RTD core dump(s)")
        else:
            # Disabled failing test run until we can sort the core files out
            raise AssertionError("Core dump(s) found")

    def __copy_to_coredump_dir(self, filepath, testname, timestamp=None):
        if timestamp is None:
            timestamp = datetime.datetime.now()
        testname = (testname.replace("/", "_") + "_" + str(timestamp)).replace(" ", "_")
        coredump_dir = "/opt/test/coredumps"
        os.makedirs(coredump_dir, exist_ok=True)
        dest = os.path.join(coredump_dir, testname)
        logger.warn("Moving core file to %s" % dest)
        shutil.copy(filepath, dest)

    def ignore_coredumps_and_segfaults(self):
        self.__m_ignore_cores_segfaults = True


def __main(argv):
    for arg in argv[1:]:
        attempt_backtrace_of_core(arg)

    c = CoreDumps()
    c.enable_core_files()
    return 0


if __name__ == "__main__":
    sys.exit(__main(sys.argv))
