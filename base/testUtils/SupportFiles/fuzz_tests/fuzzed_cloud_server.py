#!/usr/bin/env python
# coding=utf-8

import argparse
import http.server
from kitty.remote import RpcClient
import ssl

import sys
import os
import time
import PathManager

libs_dir = os.path.abspath(os.path.join(current_dir, os.pardir, os.pardir, 'SupportFiles'))
sys.path.insert(1, libs_dir)

from CloudAutomation.cloudServer import MCSRequestHandler, getServerCert, parseArguments, setupLogging, logger, \
    action_log, daemonise, deletePidFile, GL_ENDPOINTS

cloud_server_args = None
class FuzzingHttpServer(http.server.HTTPServer):
    '''
    http://docs.python.org/lib/module-BaseHTTPServer.html
    '''

    def __init__(self, server_address, handler, fuzzer):
        http.server.HTTPServer.__init__(self, server_address, handler)
        self.fuzzer = fuzzer

class MCSRequestHandlerFuzzed(MCSRequestHandler):
    """
    fuzzed handler
    """

    def __init__(self, request, client_address, server):
        super(MCSRequestHandlerFuzzed, self).__init__(request, client_address, server)

    def do_GET_mcs(self):
        stage = self.server.fuzzer.get_template_stage_name().decode()
        global GL_ENDPOINTS
        if self.path.startswith("/mcs/policy/application/MCS"):
            if "mcs_policy_fuzz" in stage:
                policy = self.get_mutated_policy(stage=stage)
                if policy is not None:
                    GL_ENDPOINTS.updatePolicy("MCS", policy)
        elif self.path.startswith("/mcs/policy/application/ALC"):
            if "alc_policy_fuzz" in stage:
                policy = self.get_mutated_policy(stage)
                if policy is not None:
                    GL_ENDPOINTS.updatePolicy("ALC", policy)

        elif self.path.startswith("/mcs/policy/application/MDR"):
            logger.info("matched MDR")
            if "mdr_policy_fuzz" in stage:
                policy = self.get_mutated_policy(stage=stage)
                if policy is not None:
                    GL_ENDPOINTS.updatePolicy("MDR", policy)

        super(MCSRequestHandlerFuzzed, self).do_GET_mcs()

    def get_mutated_policy(self, stage="mcs_policy_fuzz", data={}):
        logger.debug("policy fuzz request for stage '{}' ".format(stage))
        policy = self.server.fuzzer.get_mutation(stage=stage, data=data)
        if policy is not None:
            self.server.fuzzer.update_fuzzed_policies(stage=stage, policy=policy)
            return policy.decode()
        logger.warning("fuzzer returned none".format(stage))
        return None


def main():
    parser = argparse.ArgumentParser(description='MCS_Router fuzz testing for SSPL Linux automation')
    cloud_server_args = parseArguments(parser)
    #
    # The fuzzer process waits on port 26007
    #
    agent = RpcClient(host='localhost', port=26007)

    #
    # tell the fuzzer to start fuzzing (it will trigger connections to the http server)
    #
    agent.start()

    setupLogging()
    server = FuzzingHttpServer(('localhost', 4443), MCSRequestHandlerFuzzed, agent)
    MCSRequestHandler.options = cloud_server_args
    certfile = getServerCert()
    server.socket = ssl.wrap_socket(server.socket, certfile=certfile, server_side=True)

    while not server.fuzzer.is_done():
        server.handle_request()

    agent.stop_remote_server()


if __name__ == '__main__':
    main()
