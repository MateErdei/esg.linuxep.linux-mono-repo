import os
import sys
import time

from kitty.interfaces import WebInterface
from kitty.model import GraphModel

from local_process import LocalProcessController
import livequery_command_template as lq_command_template
import livequery_response_template as lq_response_template
from push_server_target import PushServerTarget
from livequery_response_target import LiveQueryResponseTarget
from kitty.fuzzers import ServerFuzzer
import argparse
import logging

current_dir = os.path.dirname(os.path.abspath(__file__))
libs_dir = os.path.abspath(os.path.join(current_dir, os.pardir, os.pardir, 'libs'))
sys.path.insert(1, libs_dir)


from MCSRouter import MCSRouter
import ArgParserUtils

def run_push_fuzz_test(testsuite=None, logger=None):
    parser = argparse.ArgumentParser(description='MCS_Router fuzz testing for SSPL Linux automation')
    parser.add_argument("--suite", default='livequery', type=str,
                        help="Specify push command to fuzz", choices=['livequery', 'response', 'wakeup'])
    ArgParserUtils.add_mcs_fuzz_tests_args(parser)

    fuzzer_args, _ = parser.parse_known_args()

    fuzzer = ServerFuzzer(name="PushClientFuzzer")
    fuzzer.set_interface(WebInterface(host='0.0.0.0', port=26000))
    fuzzer.set_max_failures(fuzzer_args.max_failures)
    fuzzer.set_skip_env_test(True)

    mcs_router = MCSRouter()
    target = PushServerTarget(name='PushServerTarget')

    suite = testsuite if testsuite else fuzzer_args.suite
    fuzzer.logger.info("Setting up mcs fuzzer for suite = " + suite)

    model = GraphModel()
    if suite == "livequery":
        model.connect(lq_command_template.livequery_command)
    elif suite == "wakeup":
        model.connect(lq_command_template.wakeup_command)
    elif suite == "response":
        target = LiveQueryResponseTarget(name='PushServerTarget', response_dir=os.path.join(mcs_router.mcs_dir,
                                                                            "response"), tmp_dir=mcs_router.tmp_path)
        model.connect(lq_response_template.response_fuzz)
    else:
        raise AssertionError("Unknown suite name only 'livequery', 'wakeup' are valid. Given suite {}".format(suite))

    controller = LocalProcessController('PushClientController', mcs_router.router_path, ["--no-daemon", "--console", "-v"])
    time.sleep(2)

    target.set_controller(controller)

    fuzzer.set_target(target)

    fuzzer.set_model(model)
    fuzzer.set_range(end_index=model.num_mutations() / fuzzer_args.range)
    fuzzer.set_delay_between_tests(fuzzer_args.delay_between_tests)

    stage = fuzzer.model.get_template_info()['name']

    fuzzer.logger.info("Starting fuzz for stage : {}\n".format(stage))
    fuzzer.start()

    fuzzer.logger.info('-------------- done with fuzzing -----------------')
    fuzzer.logger.info("Finished fuzz for stage : {}\n".format(stage))
    fuzzer.stop()

    report = target.get_report()

    if report.get_status() == report.PASSED:
        return 0
    elif report.get_status() == report.FAILED or report.get_status() == report.ERROR:
        fuzzer.logger.info("Failing response written to file")
        return 1

def main(argv):
    logging.basicConfig(level=logging.DEBUG)
    logger = logging.getLogger(__name__)
    try:
        return run_push_fuzz_test(logger=logger)
    except Exception as e:
        logger.error("Fuzzer failed: {}".format(str(e)))
        raise e


if __name__ == '__main__':
    sys.exit(main(sys.argv))
