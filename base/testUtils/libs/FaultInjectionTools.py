"""Keywords for FaultInjection tests """
import errno
import filecmp
import glob
import os
import re
import shutil
import subprocess

class FaultInjectionTools(object):

    def sophos_processes_related_to_plugin_registry_should_be_running_normally(self):
        def get_cmdline(pid):
            p = os.path.join("/proc", pid, "cmdline")
            try:
                return open(p).read()
            except EnvironmentError:
                return None

        processes_to_look_for = {"mcsrouter.mcs_router": 0,
                                 "sophos_managementagent": 0,
                                 "sophos_watchdog": 0}

        for pid in os.listdir("/proc"):
            cmdline = get_cmdline(pid)
            if cmdline is None:
                continue
            for procname in processes_to_look_for.keys():
                if procname in cmdline:
                    processes_to_look_for[procname] = pid
        not_running = [procname for procname, pid in processes_to_look_for.items() if pid == 0]
        if not_running:
            raise AssertionError(
                'Processes Expected to be Running but are not: {}'.format(not_running)
            )

    def ipc_file_should_exists(self, ipc_path):
        if not os.path.exists(ipc_path):
            raise AssertionError("IPC path does not exist, path: {}".format(ipc_path))

    def ipc_file_should_not_exists(self, ipc_path):
        if os.path.exists(ipc_path):
            raise AssertionError("IPC path exists, path: {}".format(ipc_path))
