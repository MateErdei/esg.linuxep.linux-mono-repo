#!/usr/bin/env python
# Copyright 2023 Sophos Limited. All rights reserved.

import logging
import time
import subprocess
import sys
import os
import socket
import shutil
from robot.libraries.BuiltIn import BuiltIn, RobotNotRunningError
try:
    import robot.api.logger as logger
    logger.warning = logger.warn
except ImportError:
    logger = logging.getLogger("UpdateServer")

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
        self.private_pem = os.path.join(PathManager.get_utils_path(), "server_certs", "server.crt")
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

    def start_simple_proxy_server(self, port):
        command = [sys.executable, os.path.join(self.server_path, "https_proxy.py"), port,
                   "--certfile", self.private_pem]
        self.proxy_processes[port] = subprocess.Popen(command, stdout=self.proxy_log, stderr=subprocess.STDOUT)
        self.wait_for_proxy_to_be_up(port)

    def start_timeout_proxy_server(self, port):
        command = [sys.executable, os.path.join(self.server_path, "https_proxy.py"), port, "--timeout-connections",
                   "--certfile", self.private_pem]
        self.proxy_processes[port] = subprocess.Popen(command, stdout=self.proxy_log, stderr=subprocess.STDOUT)

    def start_proxy_server_which_hangs_on_push_connection(self, port):
        command = [sys.executable, os.path.join(self.server_path, "https_proxy.py"), port, "--hang-on-push-connections",
                   "--certfile", self.private_pem]
        self.proxy_processes[port] = subprocess.Popen(command, stdout=self.proxy_log, stderr=subprocess.STDOUT)

    def start_proxy_server_with_digest_auth(self, port, username, password):
        command = [sys.executable, os.path.join(self.server_path, "https_proxy.py"), port, "--digest", "--username",
                   username, "--password", password , "--certfile", self.private_pem]
        self.proxy_processes[port] = subprocess.Popen(command, stdout=self.proxy_log, stderr=subprocess.STDOUT)
        self.wait_for_proxy_to_be_up(port)

    def start_proxy_server_with_basic_auth(self, port, username, password):
        command = [sys.executable, os.path.join(self.server_path, "https_proxy.py"), port, "--basic", "--username",
                   username, "--password", password,  "--certfile", self.private_pem]
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
                # Once we have verified the proxy is on the right port we now wait for it to be fully usable.
                if os.path.exists(self.__m_proxy_log_path):
                    with open(self.__m_proxy_log_path, "r") as log:
                        log_content = log.read()
                        if 'Serving HTTP Proxy' in log_content:
                            return True
            if proxyProcess.poll() is not None:
                raise AssertionError("Proxy has exited while waiting for it to be up port={}, pid={}, exit={}".format(port, proxyProcess.pid, proxyProcess.returncode))
            tries -= 1
            time.sleep(1)
        return AssertionError("Proxy hasn't started up after {} seconds port={}, pid={}".format(tries, port, proxyProcess.pid))


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


    def curl_url(self, url, proxy=None, prefix=""):
        print(f"Trying to curl {url}, proxy: {proxy}")
        # use the system curl with its library
        proxy_arg = ""
        if proxy:
            proxy_arg = f"--proxy {proxy}"
        capath = str(os.path.join(self.server_path, 'https', 'ca'))
        return os.system(f"LD_LIBRARY_PATH='' {prefix} curl -m 10 -s -k -4 --capath {capath} {proxy_arg} {url}  > /dev/null")

    def can_curl_url(self, url, proxy=None):
        if self.curl_url(url, proxy) != 0:
            raise AssertionError("cannot reach url: {}".format(url))

    def can_curl_url_as_group(self, url, proxy=None, group="sophos-spl-group"):
        # Unfortunately this only works on certain distros
        if self.curl_url(url, proxy, f"sudo --group={group}") != 0:
            raise AssertionError("cannot reach url: {}".format(url))

    def can_curl_url_as_user(self, url, proxy=None, user="sophos-spl-user"):
        if self.curl_url(url, proxy, f"sudo --user={user}") != 0:
            raise AssertionError("cannot reach url: {}".format(url))
