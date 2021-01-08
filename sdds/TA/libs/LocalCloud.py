#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (C) 2020 Sophos Plc, Oxford, England.
# All rights reserved.

import http.client
import ssl
import threading

try:
    from .CloudAutomation import cloudServer
except ImportError:
    from CloudAutomation import cloudServer

try:
    from robot.api import logger
except:
    import logging
    logger = logging.getLogger(__name__)



class CloudServerThread(threading.Thread):
    def set_server(self, options):
        self.__m_cloud_server = cloudServer.CloudServer(options)
        self.__m_ready = threading.Event()

    def startAndWaitForReady(self):
        self.__m_ready.clear()
        self.start()
        self.__m_ready.wait(1.0)

    def run(self):
        self.__m_ready.set()
        self.__m_cloud_server.run()

    def stop(self):
        logger.debug("Stopping cloud server thread")
        self.__m_cloud_server.stop()


class LocalCloud(object):
    ROBOT_LIBRARY_SCOPE = 'GLOBAL'

    def __init__(self):
        self.__m_thread = None
        self.__m_ssl_unverified_context = ssl._create_unverified_context()

    def start_local_fake_cloud(self):
        cloudServerArgs = []
        options = cloudServer.parseArguments(cloudServerArgs)
        cloudServer.HEARTBEAT_ENABLED = options.heartbeat
        cloudServer.reset_cookies()

        self.__m_thread = CloudServerThread()
        self.__m_thread.set_server(options)

        self.__m_thread.startAndWaitForReady()

    def stop_local_fake_cloud(self):
        if self.__m_thread is not None:
            logger.debug("Stopping fake cloud server thread")
            self.__m_thread.stop()
            logger.debug("Joining fake cloud server thread")
            self.__m_thread.join(10.0)
            if self.__m_thread.is_alive():
                logger.error("Failed to kill cloud server thread")
            self.__m_thread = None

    def send_policy(self, policy_type, data):
        headers = {"Content-type": "application/octet-stream", "Accept": "text/plain"}

        # Open HTTPS connection to fake cloud at https://127.0.0.1:4443
        conn = http.client.HTTPSConnection("127.0.0.1", 4443, context=self.__m_ssl_unverified_context)
        conn.request("PUT", "/controller/{}/policy".format(policy_type), data.encode('utf-8'), headers)
        conn.getresponse()
        conn.close()

    def send_policy_file(self, policy_type, policy_path):
        f = open(policy_path, 'r')
        self.send_policy(policy_type, f.read())

    def send_cmd_to_fake_cloud(self, cmd, filename=None):
        conn = http.client.HTTPSConnection("127.0.0.1", 4443, context=self.__m_ssl_unverified_context)
        conn.request("GET", "/"+cmd)
        r2 = conn.getresponse()
        logger.info('Response ' + str(r2))
        conn.close()
        if filename is not None:
            with open(filename, "wb") as f:
                f.write(r2.read())

    def trigger_update_now(self):
        self.send_cmd_to_fake_cloud("action/updatenow")
