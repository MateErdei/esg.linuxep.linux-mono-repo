#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2018-2019 Sophos Plc, Oxford, England.
# All rights reserved.



import zmq
import os
import time

from . import FakeSubscriber

COMMONCOMMUNICATIONFILESPATH=os.path.join(os.path.dirname(__file__), "..")
FakeSubscriber.addPathToSysPath(COMMONCOMMUNICATIONFILESPATH)

from PluginCommunicationTools.common.socket_utils import *
from PluginCommunicationTools.common.utils import to_byte_string

IPC_DIR = FakeSubscriber.IPC_DIR
ZMQ_CONTEXT = FakeSubscriber.ZMQ_CONTEXT

class FakePublisher(object):
    """
    FakePublisher that will connect to the pub sub data channel and send messages
    """
    def __init__(self, logger, name):
        self.logger = logger
        self.name = to_byte_string(name)

        socket_file_path = "{}/{}.ipc".format(IPC_DIR, "publisherdatachannel")
        self.publisher_socket_path = "ipc://{}".format(socket_file_path)
        self.logger.info("Socket Path {} Exists? {}".format(socket_file_path, os.path.exists(socket_file_path)))
        self.publisher_socket = try_get_socket(ZMQ_CONTEXT, self.publisher_socket_path, zmq.PUB)
        self.check_socket_connected_through_router(b"StartUp", b"Init-Start-Up-Socket")

    def __send_multipart(self, message):
        message = [ to_byte_string(s) for s in message ]
        return self.publisher_socket.send_multipart(message)


    def check_socket_connected_through_router(self, channel=b"StartUp", name=b"Start-Up-Socket"):
        """
        The below checks the the send socket is connected through the router and is used before we
        return from the publisher constructor. This makes sure that the socket is ready for sending on.
        We could have potentially used ZMQ_IMMEDIATE flag if pyzmq was compiled with libzmq > 3.3

        :param channel:
        :param name:
        :return:
        """
        full_channel = "{}_{}".format(self.name.decode("utf-8"), channel.decode("utf-8"))
        subscriber = FakeSubscriber.FakeSubscriber(self.logger, full_channel, name)
        subscriber.start()
        loop_time = 0.0
        time_out = 10
        while not subscriber.has_received_message():
            self.__send_multipart([full_channel, "Approx Wait Time On Socket Confirmation = {}".format(loop_time)])
            time.sleep(0.1)
            loop_time = loop_time + 0.1
            if time_out < loop_time:
                self.logger.error("Socket Timed Out On Connection!")
                subscriber.stop()
                raise AssertionError("Publisher Socket Timeout On Connection Test")
        subscriber.stop()

    def send_message(self, channel, data):
        self.logger.info("Sending on channel: {} message: {}".format(channel, data))
        full_message = [channel, data]
        self.__send_multipart(full_message)

    def close_socket(self):
        """
        Create a new subscriber and send a message to make sure that the last
        message from the publisher was sent successfully before closing the socket
        :return:
        """
        self.check_socket_connected_through_router(b"ShutDown", b"Shut-Down-Socket")
        self.publisher_socket.close()




