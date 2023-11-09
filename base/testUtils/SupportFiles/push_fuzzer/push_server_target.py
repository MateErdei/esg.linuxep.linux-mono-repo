#!/usr/bin/env python
# coding=utf-8

from kitty.targets.server import  ServerTarget
import os
import sys

sys.path.append('../')
import InstallPathFuzzer
sys.path.insert(1, InstallPathFuzzer.LIBS_DIR)

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

    def __del__(self):
        self._push_server_utils.shutdown_mcs_push_server()

    def _send_to_target(self, payload):
        self.logger.info("Sending payload")
        self.logger.info("%s" % payload)
        try:
            self._push_server_utils.send_message_to_push_server(payload)
        except:
            pass

    def _receive_from_target(self):
        self.not_implemented('receive_from_target')

