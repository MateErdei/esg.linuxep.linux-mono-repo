#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2018-2020 Sophos Plc, Oxford, England.
# All rights reserved.



import sys
import os
import time
import subprocess

import PathManager


SUPPORTFILEPATH = os.path.join(os.path.dirname(__file__), "..", "SupportFiles")
PathManager.addPathToSysPath(SUPPORTFILEPATH)

from PluginCommunicationTools.common.SetupLogger import setup_logging, get_log_dir
from PubSubDataChannelTools import FakePublisher

# Used for creating a single publisher in a separate process publishing
# a specified amount of data on a specified channel
def multi_send(publisher_id, data_items, channel):
    logger = setup_logging("fake_publisher_{}_{}.log".format(publisher_id, channel), "Fake Publisher {} {}".format(publisher_id, channel))
    publisher = FakePublisher.FakePublisher(logger, publisher_id)
    for datum in range(0, int(data_items)):
        data = "Data From publisher {} Num {}".format(publisher_id, datum)
        time.sleep(0.01)
        publisher.send_message(str(channel), str(data))
    publisher.close_socket()

# Used for creating publishers or sending data on multiple publishers in a
# separate process.
class FakeMultiPublisher(object):
    def __init__(self):
        self.logger = setup_logging("fake_multi_publisher.log", "Fake Multi-Publisher")
        self.publishers = []

    def add_publishers(self, number_of_publishers):
        for index in range(0, int(number_of_publishers)):
            self.publishers.append(FakePublisher.FakePublisher(self.logger, "publisher_{}".format(index)))

    def send_data(self, index, channel, data):
        self.logger.info("Sending DATA!")
        int_index = int(index)
        if int_index <= len(self.publishers):
            self.logger.info("Sending DATA {}!".format(channel))
            self.logger.info("Sending DATA {}!".format(index))
            self.logger.info(data)

            #self.logger.info("Sending {} on Channel {} on Publisher {}".format(data, channel, index))
            self.publishers[int_index - 1].send_message(channel, data)

    def check_publishers_connected(self):
        for publisher in self.publishers:
            publisher.check_socket_connected_through_router(b"StartUp", b"FakeMultiPublisher-Start-Up-Socket")

    def send_on_multiple_publishers(self, number_of_publishers, data_items, channel):
        jobs = []
        self.logger.info("Data Items value: {}".format(data_items))
        for publisher_id in range(0, number_of_publishers):
            command = ["python", str(__file__), str(publisher_id), str(data_items), str(channel)]
            with open(os.path.join(get_log_dir(), "fake_publisher_{}_{}_running.log".format(publisher_id, channel)), 'w') as stderr_stdout_log:
                jobs.append(subprocess.Popen(command, stdout=stderr_stdout_log, stderr=stderr_stdout_log))
            self.logger.info("Publisher {} Started".format(publisher_id))

        for j in jobs:
            j.wait()


if __name__ == "__main__":
    sys.exit(multi_send(sys.argv[1], sys.argv[2], sys.argv[3]))
