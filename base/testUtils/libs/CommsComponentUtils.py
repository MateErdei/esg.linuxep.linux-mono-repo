import os

COMMS_COMPONENT_EXECUTABLE_NAME="CommsComponent"

class CommsComponentUtils:
    def get_cmdline(self, pid):
        p = os.path.join("/proc", pid, "cmdline")
        try:
            return open(p).read()
        except EnvironmentError:
            return None

    def _pid_of_comms_component(self):
        pids = []
        for pid in os.listdir("/proc"):
            cmdline = self.get_cmdline(pid)
            if cmdline is None:
                continue
            if COMMS_COMPONENT_EXECUTABLE_NAME in cmdline:
                pids.append(int(pid))
        return pids

    def check_comms_component_running(self, require_running=True):
        if require_running in ['True', 'False']:
            require_running = require_running.lower() == "true"
        assert isinstance(require_running, bool)
        pids = self._pid_of_comms_component()
        pid = None
        if pids:
            if len(pids) != 1:
                raise AssertionError("Only one instance of comms component should be running at any given time. Found {}".format(pids))
            pid = pids[0]

        if pid and not require_running:
            raise AssertionError("comms component running with pid: {}".format(pid))
        if pid and require_running:
            return pid
        if not pid and not require_running:
            return None
        if not pid and require_running:
            raise AssertionError("No comms component running")