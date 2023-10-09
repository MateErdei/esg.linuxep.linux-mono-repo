import os
import time

import PathManager
import FullInstallerUtils

try:
    from robot.api import logger
except ImportError:
    import logging
    logger = logging.getLogger(__name__)


def get_websocket_server_path():
    candidates = []
    local_path_to_plugin = PathManager.find_local_component_dir_path("liveterminal")
    if local_path_to_plugin:
        candidates.append(os.path.join(local_path_to_plugin, "ta", "scripts", "utils", "websocket_server"))
    else:
        logger.error("Failed to find liveterminal when looking for websocket_server")

    candidates.append(os.path.join(FullInstallerUtils.SYSTEM_PRODUCT_TEST_INPUTS, "websocket_server"))  # vagrant location

    return FullInstallerUtils.get_plugin_sdds("websocket server", "WEBSOCKET_SERVER", candidates)


PathManager.addPathToSysPath(get_websocket_server_path())


class WebsocketWrapper:
    def __init__(self):
        self._server = None

    def start_websocket_server(self, port=443):
        import certificates
        import LTserver
        print("staring websocket server")
        self._server = LTserver.LTServerImpl('localhost', port, certificates.CERTIFICATE_PEM, log_dir=LTserver.THIS_DIR)
        self._log_path = self._server.logfile
        self._server.start()
        return self._server

    def stop_websocket_server(self):
        self._server.stop()
        self._server.join()
        self._server = None

    def __exit__(self, exc_type, exc_val, exc_tb):
        if self._server:
            self._server.stop()
            self._server.join()

    def match_message(self, message, path):
        assert self._server.match_message(message, f"/{path}"), f"Failed to match message {message} in {path}"

    def wait_for_match_message(self, message, path, timeout=10):
        start = time.time()
        while time.time() < start + timeout:
            if self._server.match_message(message, f"/{path}"):
                return
            logger.debug("Match_message returned false after %f" % (time.time() - start))
            time.sleep(0.5)
        assert self._server.match_message(message, f"/{path}"), f"Failed to match message {message} in {path}"

    def liveterminal_server_log_file(self):
        return self._log_path

    def send_message_with_newline(self, message, path):
        self._server.send_message_with_newline(message, f"/{path}")

    @staticmethod
    def install_lt_server_certificates():
        import certificates
        certificates.InstallCertificates.install_certificate()

    @staticmethod
    def uninstall_lt_server_certificates():
        import certificates
        certificates.InstallCertificates.uninstall_certificate()


if __name__ == "__main__":
    wbs = WebsocketWrapper()
    wbs.start_websocket_server()
