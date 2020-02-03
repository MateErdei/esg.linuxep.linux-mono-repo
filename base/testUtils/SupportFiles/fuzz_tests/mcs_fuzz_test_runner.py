#!/usr/bin/env python
# -*- coding: utf-8 -*-

import argparse
import os
import sys

from kitty.interfaces import WebInterface
from kitty.model import GraphModel
from kitty.remote import RpcServer

import mcsrouter_policy_templates as policy_templates
from mcsrouter_client_fuzzer import McsRouterClientFuzzer
from mcsrouter_client_controller import MCSFuzzerClientController
from mcsrouter_client_target import McsRouterTarget

current_dir = os.path.dirname(os.path.abspath(__file__))
libs_dir = os.path.abspath(os.path.join(current_dir, os.pardir, os.pardir, 'libs'))
sys.path.insert(1, libs_dir)

import ArgParserUtils


def run_mcs_fuzz_test(testsuite=None):
    parser = argparse.ArgumentParser(description='MCS_Router fuzz testing for SSPL Linux automation')
    ArgParserUtils.add_mcsrouter_fuzz_tests_args(parser)
    cloud_server_args, _ = parser.parse_known_args()
    
    fuzzer = McsRouterClientFuzzer(name='mcs-router-fuzzer')
    fuzzer.set_interface(WebInterface(host='0.0.0.0', port=26000))
    fuzzer.set_max_failures(cloud_server_args.max_failures)

    target = McsRouterTarget('McsClientTarget')

    is_robot_run = not cloud_server_args.using_python
    controller = MCSFuzzerClientController(restart_mcs=False, robot_run=is_robot_run)
    target.set_controller(controller)
    target.set_mutation_server_timeout(cloud_server_args.mutation_server_timeout)
    target.set_post_fuzz_delay(cloud_server_args.delay_between_tests)

    model = GraphModel()
    suite = testsuite if testsuite else cloud_server_args.suite
    fuzzer.logger.info("Setting up mcs fuzzer for suite = " + suite)
    if suite == "mcs":
        model.connect(policy_templates.mcs_policy_fuzzed)
    elif suite == 'alc':
        model.connect(policy_templates.alc_policy_fuzzed)
    elif suite == 'mdr':
        model.connect(policy_templates.mdr_policy_fuzzed)
    else:
        raise AssertionError("Unknown suite name only 'mcs', 'mdr' 'alc' are valid. Given suite {}".format(suite))

    fuzzer.set_model(model)
    fuzzer.set_target(target)

    fuzzer.set_range(end_index=model.num_mutations() / cloud_server_args.range)
    fuzzer.set_delay_between_tests(1.0)

    stage = fuzzer.model.get_template_info()['name']
    fuzzer.logger.info("Starting fuzz for stage : {}\n".format(stage))

    remote = RpcServer(host='localhost', port=26007, impl=fuzzer)
    remote.start()

    fuzzer.logger.info("Finished fuzz for stage : {}\n".format(stage))
    fuzzer.stop()
    report = target.get_report()

    if report.get_status() == report.PASSED:
        return 0
    elif report.get_status() == report.FAILED or report.get_status() == report.ERROR:
        fuzzer.logger.info("Failing policies written to file:{}".format(fuzzer.failed_policies_file))

        return 1


def main(argv):
    return run_mcs_fuzz_test()

if __name__ == '__main__':
    sys.exit(main(sys.argv))
