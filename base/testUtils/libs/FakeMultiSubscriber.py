#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2018-2020 Sophos Plc, Oxford, England.
# All rights reserved.


import sys
import os
import signal
import subprocess
import time

import PathManager

SUPPORTFILEPATH = PathManager.get_support_file_path()
PathManager.addPathToSysPath(SUPPORTFILEPATH)

from PluginCommunicationTools.common.SetupLogger import setup_logging, get_log_dir
from PubSubDataChannelTools import FakeSubscriber
from PubSubDataChannelTools import FakePublisher


def sighandler(signum,frame):
    raise Exception('signal received')


# Used for creating a single receiver in a separate process listening to either
# one channel or multiple channels separated by an "_"
# e.g
# channel = channel1 listens to a single channel
# channel = channel1_channel2 will listen to two channels channel1 and channel2
def multi_receive(channel, name):
    signal.signal(signal.SIGINT, sighandler)
    logger = setup_logging("{}.log".format(name), "Fake Subscriber")
    if "_" in channel:
        channels = channel.split("_")
        subscriber = FakeSubscriber.FakeSubscriber(logger, channels, name)
    else:
        subscriber = FakeSubscriber.FakeSubscriber(logger, channel, name)
    try:
        subscriber.run(in_thread=False)
    except Exception as ex:
        logger.warn("Exception on subscriber: {}".format(ex.message))
    subscriber.stop()


# Used for creating subscribers and starting the subscribers thread or running multiple subscribers in a
# separate process.
class FakeMultiSubscriber(object):
    def __init__(self):
        self.log_name = "fake_multi_subscriber.log"
        self.logger = setup_logging(self.log_name, "Fake Multi-Subscriber")
        self.subscribers = []
        self.processes = []

    def __del__(self):
        self.clean_up_multi_subscriber()

    def add_subscriber(self, channel, name, TIMEOUT=10, SLEEP_TIME=0.5):
        self.logger.info("Add subscriber for channel={}, name={}, test='{}'".format(channel, name, FakeSubscriber.testName()))

        subscriber = FakeSubscriber.FakeSubscriber(self.logger, channel, name)
        subscriber.start()
        self.subscribers.append(subscriber)

        # Perform a check that this subscriber is connected and listening before continuing
        publisher = FakePublisher.FakePublisher(self.logger, name)
        connected = False
        timeout = 0
        send_string = "Check-Connected"
        check_string = u"Sub {} received message: [b'{}', b'{}']".format(name, channel, send_string)
        path_to_log = os.path.join(get_log_dir(), self.log_name)
        while not connected and timeout < TIMEOUT:
            publisher.send_message(str(channel), send_string)
            with open(path_to_log, "r") as log:
                contents = log.read()

            if check_string in contents:
                self.logger.info("Found {} in {}".format(check_string, path_to_log))
                connected = True
                break

            else:
                connected = False
                time.sleep(SLEEP_TIME)
                timeout += SLEEP_TIME

        if not connected:
            raise AssertionError("Subscriber Not Ready To Receive Messages After {} Seconds".format(TIMEOUT))

        publisher.close_socket()
        subscriber.reset_queue()

    def get_next_message(self, index, channel):
        return self.subscribers[index].get_next_message(channel)

    def get_all_messages(self, index, channel):
        return self.subscribers[index].get_all_messages_with_10_second_timeout(channel)

    def _stop_subscriber(self, subscriber):
        try:
            subscriber.stop()
        except Exception as ex:
            self.logger.warn("Error on stopping subscriber: {}".format(ex.message))

    def _kill_process(self, process):
        try:

            os.kill(process.pid, signal.SIGINT)
        except Exception as ex:
            self.logger.warn("Error on stopping process: {}".format(ex.message))

    def clean_up_multi_subscriber(self):
        self.logger.info("Clean up multi subscribers")
        for subscriber in self.subscribers:
            self._stop_subscriber(subscriber)
        self.subscribers = []
        if self.processes:
            self.logger.info("Killing {} processes.".format(len(self.processes)))
        for process in self.processes:
            self._kill_process(process)
        self.processes = []

    def start_multiple_subscribers(self, number_of_subscribers, channel):
        for subscriber_id in range(0, number_of_subscribers):
            sub_name = "Subscriber_{}_{}".format(subscriber_id, channel)
            command = ["python", str(__file__), str(channel), str(sub_name)]
            with open(os.path.join(get_log_dir(), "{}_running.log".format(sub_name)), 'w') as stderr_stdout_log:
                self.processes.append(subprocess.Popen(command, stdout=stderr_stdout_log, stderr=stderr_stdout_log))
                self.logger.info("Created new fake subscriber process  ")
        # Perform a check that all subscribers are connected and listening before continuing
        publisher = FakePublisher.FakePublisher(self.logger, "Test-Multi-Subscribers")
        connected = False
        timeout = 0
        while not connected and timeout < 10:
            send_string = "Check-Connected"
            publisher.send_message(str(channel), send_string)
            connected = True
            for subscriber_id in range(0, number_of_subscribers):
                sub_name = "Subscriber_{}_{}.log".format(subscriber_id, channel)
                path_to_log = os.path.join(get_log_dir(), sub_name)
                try:
                    with open(path_to_log, "r") as log:
                        contents = log.read()
                        #Filter non-ascii characters
                        contents = contents.decode('ascii', 'ignore')
                        if send_string not in contents:
                            connected = False
                            break
                except IOError:
                    self.logger.info("Can't Find File : {}".format(path_to_log))
                    connected = False
                    continue
            if not connected:
                time.sleep(0.5)
                timeout += 0.5
        if not connected:
            raise AssertionError("Multiple Subscribers Not Ready To Receive Messages After 10 Seconds")
        publisher.close_socket()


if __name__ == "__main__":
    sys.exit(multi_receive(sys.argv[1], sys.argv[2]))