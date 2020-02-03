#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2018-2019 Sophos Plc, Oxford, England.
# All rights reserved.



import zmq
import sys
import os
import time
import queue
from threading import Thread
from robot.libraries.BuiltIn import BuiltIn, RobotNotRunningError
import traceback

def addPathToSysPath(p):
    p = os.path.normpath(p)
    if p not in sys.path:
        sys.path.append(p)

COMMONCOMMUNICATIONFILESPATH=os.path.join(os.path.dirname(__file__), "..")
addPathToSysPath(COMMONCOMMUNICATIONFILESPATH)

from PluginCommunicationTools.common.SetupLogger import *
from PluginCommunicationTools.common.socket_utils import *
import PluginCommunicationTools.common.IPCDir
IPC_DIR = PluginCommunicationTools.common.IPCDir.IPC_DIR
import PluginCommunicationTools.common.socket_utils
ZMQ_CONTEXT = PluginCommunicationTools.common.socket_utils.ZMQ_CONTEXT

SUBSCRIBER_COUNT = 0

def testName():
    try:
        return BuiltIn().get_variable_value("${TEST NAME}")
    except RobotNotRunningError:
        return None


class FakeSubscriber(object):
    """
    FakeSubscriber that will connect to the pub sub data channel.
    Start the thread to listen to the socket and write received messages to file.
    """
    def __init__(self, logger, subscribe_to, name):
        self.name = name
        self.logger = logger
        self.__m_original_test_name = testName()
        socket_file_path = "{}/{}.ipc".format(IPC_DIR, "subscriberdatachannel")
        self.subscriber_socket_path = "ipc://{}".format(socket_file_path)
        self.logger.info("Socket Path {} Exists? {}".format(socket_file_path, os.path.exists(socket_file_path)))
        self.subscriber_socket = PluginCommunicationTools.common.socket_utils.try_get_socket(ZMQ_CONTEXT, self.subscriber_socket_path, zmq.SUB)
        self.logger.info("Socket created {}".format(name))
        if isinstance(subscribe_to, list):
            for channel_name in subscribe_to:
                self.subscriber_socket.setsockopt(zmq.SUBSCRIBE, channel_name.encode())
        else:
            self.subscriber_socket.setsockopt(zmq.SUBSCRIBE, subscribe_to.encode())
        self.logger.info("Subscription configured {}".format(name))
        self.__m_running = False
        self.run_thread = None
        self.received_first_message = False
        self.queues = {}
        self.logger.info("Fakesubscriber constructed {}".format(name))

    def start(self):
        if not self.__m_running:
            global SUBSCRIBER_COUNT
            SUBSCRIBER_COUNT += 1
            self.__m_running = True
            self.run_thread = Thread(target=self.run)
            self.logger.info("Starting Subscriber {} ({} running) (for '{}')".format(self.name, SUBSCRIBER_COUNT, self.__m_original_test_name))
            self.run_thread.start()

    def stop(self):
        global SUBSCRIBER_COUNT
        if self.__m_running:
            SUBSCRIBER_COUNT -= 1
            self.__m_running = False
            self.logger.info("Stopping Subscriber {} ({} running) (for '{}')".format(self.name, SUBSCRIBER_COUNT, self.__m_original_test_name))
            if self.run_thread is not None:
                self.run_thread.join()
            self.run_thread = None
        else:
            self.logger.error("Unexpected stopping non-running subscriber {} ({} running) (for '{}')".format(self.name, SUBSCRIBER_COUNT, self.__m_original_test_name))

    def run(self, in_thread=True):
        if not in_thread:
            self.__m_running = True
        self.logger.info("Subscriber run method")
        poller = zmq.Poller()
        poller.register(self.subscriber_socket, zmq.POLLIN)
        count = 0
        while self.__m_running:
            count += 1
            try:
                # milliseconds - https://pyzmq.readthedocs.io/en/latest/api/zmq.html#zmq.Poller.poll
                sockets = dict(poller.poll(500))

                if count % 10 == 0:
                    self.logger.info("subscriber running poller returned: {} ".format(repr(sockets)))

            except KeyboardInterrupt:
                break
            if self.subscriber_socket in sockets:
                self.received_first_message = True
                self.handle_receive_message()
            if self.__m_original_test_name != testName():
                self.logger.error("Aborting Subscriber as test name has changed to {} from {}".format(
                    testName(), self.__m_original_test_name))
                break
        self.logger.info("Ended Listening Loop on {}".format(self.name))

    def has_received_message(self):
        return self.received_first_message

    def handle_receive_message(self):
        message_data = self.subscriber_socket.recv_multipart()
        if message_data[0] not in list(self.queues.keys()):
            self.queues[message_data[0]] = queue.Queue()
        self.queues[message_data[0]].put((message_data[1]))
        self.logger.info("Sub {} received message: {}".format(self.name, message_data))

    def reset_queue(self):
        self.queues = {}

    def get_next_message(self, channel, timeout_seconds=15):
        timeout = 0
        while timeout < timeout_seconds:
            if channel in list(self.queues.keys()):
                return self.queues[channel].get(block=True, timeout=timeout_seconds)
            time.sleep(0.5)
            timeout += 0.5
        raise AssertionError("No message queue available on channel: {}".format(channel))

    # This timeout is to allow for systems where there are messages arriving regularly.
    # The timeout will crop the collected messages to those that have arrived or arrive
    # in the ten seconds after it is called
    def get_all_messages_with_10_second_timeout(self, channel):
        messages = []
        start_time = time.time()
        while time.time() < start_time + 10:
            try:
                messages.append(self.get_next_message(channel, 3))
            except queue.Empty:
                break

        return messages

