import os
import PathManager
import FullInstallerUtils

def get_websocket_server_path():
    candidates = []
    local_path_to_plugin = PathManager.find_local_component_dir_path("liveterminal")
    if local_path_to_plugin:
        candidates.append(os.path.join(local_path_to_plugin, "ta", "scripts", "utils", "websocket_server"))
    return FullInstallerUtils.get_plugin_sdds("websocket server", "WEBSOCKET_SERVER", candidates)

PathManager.addPathToSysPath(get_websocket_server_path())
import LTserver, certificates

class WebsocketWrapper:
    def __init__(self):
        self._server = None

    def start_websocket_server(self, port=443):
        print("staring websocket server")
        self._server = LTserver.LTServerImpl('localhost', port, certificates.CERTIFICATE_PEM, log_dir="/tmp/")
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
        assert self._server.match_message(message,path)

    def send_message_with_newline(self, message, path):
        self._server.send_message_with_newline(message, path)


if __name__ == "__main__":
    wbs = WebsocketWrapper()
    wbs.start_websocket_server()
