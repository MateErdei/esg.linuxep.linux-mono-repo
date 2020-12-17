#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (C) 2020 Sophos Plc, Oxford, England.
# All rights reserved.

import os
import subprocess
import time

from robot.libraries.BuiltIn import BuiltIn, RobotNotRunningError
try:
    from robot.api import logger
except:
    import logging
    logger = logging.getLogger(__name__)


try:
    from . import cidServer
    from . import PathManager
except ImportError:
    import cidServer
    import PathManager

SUPPORT_FILES = PathManager.get_support_file_path()
HTTPS_FILES = os.path.join(SUPPORT_FILES, "https")


def _get_variable(var_name, default_value=None):
    try:
        var = BuiltIn().get_variable_value("${}".format(var_name))
        if var is not None:
            return var
    except RobotNotRunningError:
        pass

    return os.environ.get(var_name, default_value)


class UpdateServer(object):
    def __init__(self):
        self.private_pem = os.path.join(HTTPS_FILES, "server-private.pem")
        self.__m_servers = []

    def start_update_server(self, *options):
        self.generate_update_certs()
        options = cidServer.parseOptions(list(options))
        server = cidServer.CidServerThread()
        server.set_options(options)
        server.startAndWait()
        self.__m_servers.append(server)

    def stop_update_servers(self):
        logger.info("Stopping servers")
        for server in self.__m_servers:
            server.stop()
        logger.info("Joining servers")
        for server in self.__m_servers:
            server.join()
        self.__m_servers = []

    def generate_update_certs(self):
        # certs are valid for 24h, regenerate if they're more than 12h old.
        if os.path.exists(self.private_pem):
            age = time.time() - os.stat(self.private_pem).st_ctime
            if age < 12 * 60 * 60:
                # certificate is less than 12h old
                return

        openssl_bin_path = _get_variable("OPENSSL_BIN_PATH", "/usr/bin")
        openssl_lib_path = _get_variable("OPENSSL_LIB_PATH", "/lib64")
        if openssl_bin_path is None or openssl_lib_path is None:
            raise AssertionError("openssl environment variables not set OPENSSL_BIN_PATH={}, OPENSSL_LIB_PATH={}".format(openssl_bin_path, openssl_lib_path))

        env=os.environ.copy()
        env["PATH"] = openssl_bin_path + ":" + env["PATH"]
        env["LD_LIBRARY_PATH"] = openssl_lib_path

        subprocess.check_call(["make", "-C", HTTPS_FILES, "clean"], env=env, stdout=subprocess.PIPE)
        subprocess.check_call(["make", "-C", HTTPS_FILES, "all"], env=env, stdout=subprocess.PIPE)
