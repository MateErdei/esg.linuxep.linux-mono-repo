#!/usr/bin/env python3
# Copyright 2019 Sophos Plc, Oxford, England.

"""
atomic_write Module
"""
import grp
import os
import codecs
import pwd
import uuid
import shutil

from kitty.targets.server import  ServerTarget

import sys

current_dir = os.path.dirname(os.path.abspath(__file__))
libs_dir = os.path.abspath(os.path.join(current_dir, os.pardir, os.pardir, 'libs'))
sys.path.insert(1, libs_dir)

from LogUtils import LogUtils

class LiveQueryResponseTarget(ServerTarget):
    '''
    This class represents a target when fuzzing a server.
    Its main job, beside using the Controller and Monitors, is to send and
    receive data from/to the target.
    '''

    def __init__(self, name, logger=None, expect_response=False, response_dir=None, tmp_dir=None):
        '''
        :param name: name of the target
        :param logger: logger for this object (default: None)
        :param expect_response: should wait for response from the victim (default: False)
        '''
        super(LiveQueryResponseTarget, self).__init__(name, logger, expect_response)
        self.last_served_response = None
        self._log_utils = LogUtils()
        self._response_dir = response_dir
        if not response_dir:
            self._response_dir = "/opt/sophos-spl/base/mcs/response"
        self._tmp_dir = tmp_dir
        if not self._tmp_dir:
            self._tmp_dir = "/tmp"
        self._spl_gid = grp.getgrnam("sophos-spl-group").gr_gid
        self._spl_uid = pwd.getpwnam("sophos-spl-user").pw_uid

    def post_test(self, test_num):
        super(LiveQueryResponseTarget, self).post_test(test_num)
        try:
            self._log_utils.check_mcsrouter_does_not_contain_critical_exceptions()
        except Exception as ex:
            super(LiveQueryResponseTarget, self).get_report().failed(str(ex))
            #grab last served response for retest
            if self.last_served_response:
                path = os.path.join("./kittylogs/", self.last_served_response["filename"])
                with codecs.open(path, mode="wb") as file_to_write:
                    file_to_write.write(self.last_served_response["response"])

    def _send_to_target(self, payload):
        self.logger.info("Sending payload")
        self.logger.info("%s" % payload)

        filename = "LiveQuery_{}_response.json".format(str(uuid.uuid4()))
        path = os.path.join(self._response_dir, filename)
        tmp_path = os.path.join(self._tmp_dir, filename)
        try:
            self.atomic_write(path, tmp_path, data=payload)
            self.last_served_response = {"filename": filename, "response": payload}
        except Exception as ex:
            self.logger.error("Atomic write failed with message: %s", str(ex))

    def _receive_from_target(self):
        self.not_implemented('receive_from_target')

    def atomic_write(self, path, tmp_path, data):
        """
        atomic_write
        :param path:
        :param tmp_path:
        :param data:
        """
        try:
             with codecs.open(tmp_path, mode="wb") as file_to_write:
                 file_to_write.write(data)
             os.chown(tmp_path, uid=self._spl_uid, gid=self._spl_gid)
             os.chmod(tmp_path, 600)
             shutil.move(tmp_path, path)
        except (OSError, IOError) as exception:
            self.logger.error("Atomic write failed with message: %s", str(exception))