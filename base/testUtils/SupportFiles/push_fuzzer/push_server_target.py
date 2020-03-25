#!/usr/bin/env python
# coding=utf-8

from kitty.targets.server import  ServerTarget
import os
import sys

current_dir = os.path.dirname(os.path.abspath(__file__))
libs_dir = os.path.abspath(os.path.join(current_dir, os.pardir, os.pardir, 'libs'))
sys.path.insert(1, libs_dir)

from PushServerUtils import PushServerUtils

class PushServerTarget(ServerTarget):
    '''
    This class represents a target when fuzzing a server.
    Its main job, beside using the Controller and Monitors, is to send and
    receive data from/to the target.
    '''

    def __init__(self, name, logger=None, expect_response=False):
        '''
        :param name: name of the target
        :param logger: logger for this object (default: None)
        :param expect_response: should wait for response from the victim (default: False)
        '''
        super(PushServerTarget, self).__init__(name, logger, expect_response)
        self._push_server_utils = PushServerUtils()
        self._push_server_utils.start_mcs_push_server()
        self.last_served_response = None

    def __del__(self):
        self._push_server_utils.shutdown_mcs_push_server()

    def _send_to_target(self, payload):
        self.logger.info("Sending payload")
        self.logger.info("%s" % payload)
        try:
            self._push_server_utils.send_message_to_push_server(payload)
            self.last_served_response = {"last_served_push_command", payload}
        except:
            pass

    def _receive_from_target(self):
        self.not_implemented('receive_from_target')

