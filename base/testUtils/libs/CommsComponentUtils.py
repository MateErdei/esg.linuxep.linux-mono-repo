import os
import json
import base64

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
            # TODO - LINUXDAR-1954: This will fail when there is a parent and child comms process. Change this keyword to look for the new expected behaviour
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
        
    
