import os
import time
import uuid

import PathManager
import FullInstallerUtils

try:
    from robot.api import logger
except ImportError:
    import logging
    logger = logging.getLogger(__name__)


def get_websocket_server_path():

    dir_path = FullInstallerUtils.get_plugin_sdds("websocket server", "WEBSOCKET_SERVER")
    return os.path.join(dir_path, "utils", "websocket_server")


PathManager.addPathToSysPath("/opt/test/inputs/pytest_scripts/utils/websocket_server")  # TAP location
PathManager.addPathToSysPath(get_websocket_server_path())




class WebsocketWrapper:
    def __init__(self):
        self._server = None

    def start_websocket_server(self, port=443):
        assert self._server is None
        import certificates
        import LTserver
        print("staring websocket server")
        self._server = LTserver.LTServerImpl('localhost', port, certificates.CERTIFICATE_PEM, log_dir=LTserver.THIS_DIR)
        self._log_path = self._server.logfile
        print(self._log_path)
        self._server.start()
        return self._server

    def stop_websocket_server(self):
        assert self._server is not None
        self._server.stop()
        self._server.join()
        self._server = None

    def __del__(self):
        if self._server:
            self._server.stop()
            self._server.join()

    def wait_for_check_that_command_prompt_has_started(self, path, timeout=10):
        assert self._server
        start = time.time()
        while time.time() < start + timeout:
            if self._server.check_that_command_prompt_has_started(path):
                return
            logger.debug("check_that_command_prompt_has_started returned false after %f" % (time.time() - start))
            time.sleep(0.5)
        self._server.check_that_command_prompt_has_started(path)

    def match_message(self, message, path, timeout=1):
        assert self._server
        if not self._server.match_message(message, f"/{path}", timeout=timeout):
            raise AssertionError(f"Failed to match message {message} in {path}")

    def wait_for_match_message(self, message, path, timeout=10):
        assert self._server
        start = time.time()
        while time.time() < start + timeout:
            if self._server.match_message(message, f"/{path}", timeout=timeout):
                return
            logger.debug("Match_message returned false after %f" % (time.time() - start))
            time.sleep(0.5)
        self.match_message(message, path)

    def liveterminal_server_log_file(self):
        return self._log_path

    def send_message_with_newline(self, message, path):
        assert self._server
        self._server.send_message_with_newline(message, f"/{path}")

    @staticmethod
    def install_lt_server_certificates():
        import certificates
        certificates.InstallCertificates.install_certificate()

    @staticmethod
    def uninstall_lt_server_certificates():
        import certificates
        certificates.InstallCertificates.uninstall_certificate()

    @staticmethod
    def generate_random_path():
        return str(uuid.uuid4())

    def get_log_path(self):
        return self._log_path

if __name__ == "__main__":
    wbs = WebsocketWrapper()
    wbs.start_websocket_server()
