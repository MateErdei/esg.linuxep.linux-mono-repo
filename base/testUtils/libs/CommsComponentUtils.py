import os
import json
import base64
import psutil

from robot.api import logger

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
            logger.info(pid)
            cmdline = self.get_cmdline(pid)
            if cmdline is None:
                continue
            if COMMS_COMPONENT_EXECUTABLE_NAME in cmdline:
                pids.append(int(pid))
        return pids

    def get_pid_of_comms(self, network_or_local=None):
        if network_or_local not in ["network","local"]:
            raise AssertionError("need to pick either network or local")
        pids = self._pid_of_comms_component()
        assert len(pids) == 2, "Expected two pids"
        #sort ascending
        pids.sort()
        logger.debug(f"pids: {pids}")
        network_pid = pids[1]
        logger.info(f"Got network PID as {network_pid}")
        local_pid = pids[0]
        logger.info(f"Got local PID as {local_pid}")
        p = psutil.Process(network_pid)
        parent = p.ppid()
        assert parent == local_pid, f"expected {local_pid} to be parent of {network_pid}"
        if network_or_local == "local":
            return local_pid
        if network_or_local == "network":
            return network_pid

    def check_comms_component_running(self, require_running=True):
        if require_running in ['True', 'False']:
            require_running = require_running.lower() == "true"
        assert isinstance(require_running, bool)
        pids = self._pid_of_comms_component()
        pid = None
        if pids:
            if len(pids) != 2:
                raise AssertionError("Expected two commscomponent processes. Found {}".format(pids))
            pid = pids[0]

        if pid and not require_running:
            raise AssertionError("comms component running with pid: {}".format(pid))
        if pid and require_running:
            return pid
        if not pid and not require_running:
            return None
        if not pid and require_running:
            raise AssertionError("No comms component running")
    
    def create_http_json_request(self, filename, **kwargs):
        """
        Define how to create the json request, any of the following parameters can be passed:
        requestType
        server
        resourcePath
        bodyContent
        port
        headers
        certPath
        """
        entry = dict(kwargs)
        bodyContent = 'bodyContent'
        if bodyContent in entry:
            entry[bodyContent] = base64.b64encode( entry[bodyContent].encode() ).decode()
        with open(filename, 'w') as f:
            json.dump(entry,f)

    def extract_bodyContent_of_json_response(self,filename, httpCode=""):
        with open(filename,'r') as f:
            content = json.load(f)
            if (httpCode != ""):
                if content['httpCode'] != httpCode:
                    raise RuntimeError("Expected code does not match: Extrated: {}, Expected {}".format(content['httpCode'],httpCode))
            return base64.b64decode(content['bodyContent']).decode(errors='replace')
        return ""
        
    
