import time
import subprocess
import sys
import os
import socket
import shutil
from robot.libraries.BuiltIn import BuiltIn, RobotNotRunningError
import robot.api.logger as logger

import PathManager


LOCALHOSTS = """dci.sophosupd.com 127.0.0.1
dci.sophosupd.net 127.0.0.1
d1.sophosupd.com 127.0.0.1
d2.sophosupd.com 127.0.0.1
d3.sophosupd.com 127.0.0.1
d1.sophosupd.net 127.0.0.1
d2.sophosupd.net 127.0.0.1
d3.sophosupd.net 127.0.0.1
"""

def get_variable(varName, defaultValue=None):
    try:
        return BuiltIn().get_variable_value("${}".format(varName))
    except RobotNotRunningError:
        return os.environ.get(varName, defaultValue)


class UpdateServer(object):
    def __init__(self, server_log="update_server.log"):
        self.server_path = PathManager.get_support_file_path()
        self.private_pem = os.path.join(self.server_path, "https", "server-private.pem")
        self.server_processes = []
        self.proxy_processes = {}

        tmp_path = os.path.join(".", "tmp")
        if not os.path.exists(tmp_path):
            os.makedirs(tmp_path)

        self.server_log = open(os.path.join(tmp_path, server_log), 'w+')
        self.__m_proxy_log_path = os.path.join(tmp_path, "proxy_server.log")
        self.proxy_log = open(self.__m_proxy_log_path, 'w+')
        self.devnull = open(os.devnull, 'w')

    def __del__(self):
        # fail-safe cleanup. Tests should be doing their own cleanup, rather than relying on this.
        if self.server_processes:
            print("Killing update server(s)")
            self.stop_update_server()

        if self.proxy_processes:
            print("Killing proxy server(s)")
            self.stop_proxy_servers()

    def generate_update_certs(self):
        # certs are valid for 24h, regenerate if they're more than 12h old.
        if os.path.exists(self.private_pem):
            age = time.time() - os.stat(self.private_pem).st_ctime
            if age < 12 * 60 * 60:
                # certificate is less than 12h old
                return

        openssl_bin_path = get_variable("OPENSSL_BIN_PATH")
        openssl_lib_path = get_variable("OPENSSL_LIB_PATH")
        if openssl_bin_path is None or openssl_lib_path is None:
            raise AssertionError("openssl environment variables not set OPENSSL_BIN_PATH={}, OPENSSL_LIB_PATH={}".format(openssl_bin_path, openssl_lib_path))

        env=os.environ.copy()
        env["PATH"] = openssl_bin_path + ":" + env["PATH"]
        env["LD_LIBRARY_PATH"] = openssl_lib_path

        subprocess.check_call(["make", "-C", os.path.join(self.server_path, "https"), "clean"], env=env, stdout=subprocess.PIPE)
        subprocess.check_call(["make", "-C", os.path.join(self.server_path, "https"), "all"], env=env, stdout=subprocess.PIPE)

    def start_update_server(self, port, directory):
        if not os.path.isdir(directory):
            raise AssertionError("Trying to serve a non-existent directory: {}".format(directory))
        command = [sys.executable, os.path.join(self.server_path, "cidServer.py"), str(port), directory, "--loggingOn",
                   "--tls1_2", "--secure={}".format(self.private_pem)]
        print("Start Update Server: command: " + str(command))
        process = subprocess.Popen(command, stdout=self.server_log, stderr=subprocess.STDOUT)
        self.server_processes.append(process)
        logger.info("started update server with pid: {}".format(process.pid))
        self.wait_for_server_up("https://localhost", str(port))

    def stop_update_server(self):
        for process in self.server_processes:
            process.kill()
        self.server_processes = []

    def modify_host_file_for_local_updating(self, new_hosts_file_content=LOCALHOSTS, backup_filename="hosts.bk"):
        logger.info("adding '{}' to hosts file".format(new_hosts_file_content))
        host_file_backup = os.path.join("/etc", backup_filename)
        host_file = os.path.join("/etc", "hosts")
        shutil.copyfile(host_file, host_file_backup)
        with open(host_file, "a") as f:
            f.write(new_hosts_file_content)

    def restore_host_file(self, backup_filename="hosts.bk"):
        host_file_backup = os.path.join("/etc", backup_filename)
        host_file = os.path.join("/etc", "hosts")
        if os.path.exists(host_file_backup):
            os.rename(host_file_backup, host_file)

    def start_simple_proxy_server(self, port):
        command = [sys.executable, os.path.join(self.server_path, "https_proxy.py"), port]
        self.proxy_processes[port] = subprocess.Popen(command, stdout=self.proxy_log, stderr=subprocess.STDOUT)
        self.wait_for_proxy_to_be_up(port)

    def start_timeout_proxy_server(self, port):
        command = [sys.executable, os.path.join(self.server_path, "https_proxy.py"), port, "--timeout-connections"]
        self.proxy_processes[port] = subprocess.Popen(command, stdout=self.proxy_log, stderr=subprocess.STDOUT)

    def start_proxy_server_with_digest_auth(self, port, username, password):
        command = [sys.executable, os.path.join(self.server_path, "https_proxy.py"), port, "--digest", "--username",
                   username, "--password", password]
        self.proxy_processes[port] = subprocess.Popen(command, stdout=self.proxy_log, stderr=subprocess.STDOUT)
        self.wait_for_proxy_to_be_up(port)

    def start_proxy_server_with_basic_auth(self, port, username, password):
        command = [sys.executable, os.path.join(self.server_path, "https_proxy.py"), port, "--basic", "--username",
                   username, "--password", password]
        self.proxy_processes[port] = subprocess.Popen(command, stdout=self.proxy_log, stderr=subprocess.STDOUT)
        self.wait_for_proxy_to_be_up(port)

    def stop_proxy_server_on_port(self, proxy_port):
        proxy = self.proxy_processes.pop(proxy_port, None)
        if proxy is None:
            raise AssertionError("Proxy with port {} not started".format(proxy_port))
        else:
            proxy.kill()

    def stop_proxy_servers(self):
        for proxy in self.proxy_processes.values():
            proxy.kill()
        self.proxy_processes = {}

    def is_proxy_up(self, port):
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            s.connect(("localhost", int(port)))
            s.close()
            return True
        except EnvironmentError:
            return False

    def wait_for_proxy_to_be_up(self, port):
        proxyProcess = self.proxy_processes[port]
        tries = 100
        while tries > 0:
            if self.is_proxy_up(port):
                return True
            if proxyProcess.poll() is not None:
                raise AssertionError("Proxy has exited while waiting for it to be up port={}, pid={}, exit={}".format(port, proxyProcess.pid, proxyProcess.returncode))
            tries -= 1
            time.sleep(0.1)
        return AssertionError("Proxy hasn't started up after 10 seconds port={}, pid={}".format(port, proxyProcess.pid))


    def wait_for_server_up(self, url, port):
        tries = 0
        while self.curl_url(url + ":" + port) != 0 and tries < 100:
            tries += 1
            time.sleep(0.1)
        if tries == 100:
            #run curl in verbose mode, this produces output that will be displayed in robot logs.
            self.verbose_curl_url(url + ":" + port)
            content = ""
            try:
                content = open(self.__m_proxy_log_path, 'r').read()
            except:
                pass
            return_codes = ["Processes code: " + str(popen.poll()) for popen in self.server_processes]
            raise AssertionError("No server running on: " + str(url) + ":" + str(port) + '\n'
                                 + '\n'.join(return_codes)
                                 + '\nLogOutput: ' + content
                                 )

    def verbose_curl_url(self, url):
        command = "LD_LIBRARY_PATH='' curl -s --verbose -4 --capath " + str(os.path.join(self.server_path, "https", "ca")) + " " + str(url)
        # use the system curl with its library
        print(("Run: " + command))
        return os.system(command)



    def curl_url(self, url):
        print(("Trying to curl {}".format(url)))
        # use the system curl with its library
        return os.system(
            "LD_LIBRARY_PATH='' curl -s -k -4 --capath " + str(os.path.join(self.server_path, "https", "ca")) + " " + str(
                url) + " > /dev/null")

    def can_curl_url(self, url):
        if self.curl_url(url) != 0:
            raise AssertionError("cannot reach url: {}".format(url))

    def unpack_openssl(self, tmp_path = "/tmp"):
        openssl_input = get_variable("OPENSSL_INPUT")
        if openssl_input is None:
            raise AssertionError("Required env variable 'OPENSSL_INPUT' is not specified")
        target_path = os.path.join(tmp_path, "openssl")
        if os.path.isdir(openssl_input):
            if not os.path.isdir(target_path):
                shutil.copytree(openssl_input, target_path)


        openssl_tar = os.path.join(openssl_input, "openssl.tar")
        os.system("tar -xvf {} -C {} > /dev/null".format(openssl_tar, tmp_path))

        return target_path
