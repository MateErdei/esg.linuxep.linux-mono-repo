# Copyright 2023 Sophos Limited. All rights reserved.

import threading
import os
import grp
import time

import robot.api.logger as logger


_excluded_exe_paths = [
    # SulDownloader runs installers with root group, so some of the executables under /opt/sophos-spl will be run with
    # root group, but they don't use networking
    "/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component/files/base/bin/versionedcopy",
    "/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component/files/base/bin/machineid",
    "/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component/files/base/bin/manifestdiff",
    "/opt/sophos-spl/base/bin/versionedcopy.0",
    "/opt/sophos-spl/base/bin/machineid.0",
    "/opt/sophos-spl/base/bin/manifestdiff.0",
    "/opt/sophos-spl/plugins/av/sbin/sync_versioned_files.0",
    "/opt/sophos-spl/bin/wdctl.0",
    # Other executables
    "/opt/sophos-spl/base/bin/sophos_watchdog.0",  # Expected to use root group, doesn't use networking
    "/opt/sophos-spl/plugins/av/bin/avscanner.0",  # May be invoked by user as root, doesn't use networking
]


def _record_process_groups(stop_event, pid_map):
    while not stop_event.is_set():
        proc_dirs = os.listdir("/proc")
        for proc_dir in proc_dirs:
            if proc_dir.isdecimal():
                pid = proc_dir
                try:
                    exe = os.readlink(os.path.join("/proc", pid, "exe"))
                    if exe.startswith("/opt/sophos-spl"):
                        statinfo = os.stat(os.path.join("/proc", pid))
                        gid = statinfo.st_gid
                        pid_map[pid] = (exe, gid)
                except FileNotFoundError:
                    pass
                except PermissionError:
                    pass
                except ProcessLookupError:
                    pass
        time.sleep(0.01)


class ProcessGroupCheck:
    def __init__(self):
        self.stop_event = None
        self.thread = None
        self.pid_map = {}

    def __del__(self):
        if not self.thread:
            return
        self.stop_event.set()
        self.thread.join()

    def start_process_group_check(self):
        if self.thread:
            return
        self.pid_map = {}
        self.stop_event = threading.Event()
        self.thread = threading.Thread(target=_record_process_groups, args=(self.stop_event, self.pid_map), daemon=True)
        self.thread.start()

    def finish_process_group_check(self):
        if not self.thread:
            return

        self.stop_event.set()
        self.thread.join()

        self.stop_event = None
        self.thread = None

        spl_group_gid = grp.getgrnam("sophos-spl-group").gr_gid

        has_error = False
        for (exe, gid) in self.pid_map.values():
            logger.debug(f"Recorded {exe} with {gid} ({grp.getgrgid(gid).gr_name})")
            if gid != spl_group_gid and exe not in _excluded_exe_paths:
                logger.error(f"{exe} has GID {gid} ({grp.getgrgid(gid).gr_name}) rather than {spl_group_gid} (sophos-spl-group)")
                has_error = True

        self.pid_map = {}

        if has_error:
            raise Exception("At least one process in /opt/sophos-spl doesn't have the sophos-spl-group group")
