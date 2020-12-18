#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (C) 2020 Sophos Plc, Oxford, England.
# All rights reserved.

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
    def __init__(self):
        self.__m_thread = None

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
