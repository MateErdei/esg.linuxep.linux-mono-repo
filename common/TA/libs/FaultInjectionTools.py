"""Keywords for FaultInjection tests """
import json
import os


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
            raise AssertionError(f'Processes Expected to be Running but are not: {not_running}')

    def ipc_file_should_exists(self, ipc_path):
        if not os.path.exists(ipc_path):
            raise AssertionError(f"IPC path does not exist, path: {ipc_path}")

    def ipc_file_should_not_exists(self, ipc_path):
        if os.path.exists(ipc_path):
            raise AssertionError(f"IPC path exists, path: {ipc_path}")

    def remove_key_from_json_file(self, key, json_file):
        if not os.path.isfile(json_file):
            raise AssertionError(f"File not found {json_file}")
        with open(json_file, "r") as f:
            content = json.load(f)
            if key in content:
                content.pop(key)

        with open(json_file, "w") as f:
            json.dump(content, f)

    def modify_value_in_json_file(self, key, new_value, json_file):
        if not os.path.isfile(json_file):
            raise AssertionError(f"File not found {json_file}")
        with open(json_file, "r") as f:
            content = json.load(f)
            if key in content:
                content[key] = new_value

        with open(json_file, "w") as f:
            json.dump(content, f)
